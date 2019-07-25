//------------------------------------------------------------------------------
//
//	Copyright (C) 2016 Nexell Co. All Rights Reserved
//	Nexell Co. Proprietary & Confidential
//
//	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module		:
//	File		:
//	Description	:
//	Author		:
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <NX_QuickRearCam.h>
#include <sys/time.h>
#include <nx-v4l2.h>
#include <drm_fourcc.h>
#include <fcntl.h>

#include <NX_BackGearDetect.h>
#include <NX_CRGBDraw.h>
#include <NX_CCommand.h>

// #define NX_DBG_OFF

#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_QuickRearCam]"
#include <NX_DbgMsg.h>

#define ARGB_COLOR_RED		0xA0FF0000
#define ARGB_COLOR_GREEN	0xA000FF00
#define ARGB_COLOR_BLUE		0xA00000FF
#define ARGB_COLOR_WHITE	0xA0FFFFFF
#define ARGB_COLOR_YELLOW	0xA0FFFF00
#define ARGB_COLOR_BLACK	0xFF000000
#define ARGB_COLOR_DEFAULT	0x00000000

//#define CHECK_TIME
//#define AGING_TEST

#define STOP_COMMAND_CTRL_FILE_PATH	"/product/rearcam_cmd"
#define STATUS_CTRL_FILE_PATH		"/product/rearcam_status"

#define PGL_H_LINE_NUM	3
#define PGL_V_LINE_NUM	6

typedef struct tagHLINE_INFO{
	int32_t dsp_width;
	int32_t start_x;
	int32_t start_y;
	int32_t length;
	int32_t thickness;
	uint32_t color;
}HLINE_INFO;

typedef struct tagVLINE_INFO{
	int32_t dsp_width;
	int32_t start_x;
	int32_t start_y;
	int32_t end_y;
	int32_t thickness;
	int32_t offset_x;
	int32_t offset_y;
	uint32_t color;
}VLINE_INFO;


static void HorizontalLine(uint32_t *pBuffer, HLINE_INFO *m_Info)
{
	uint32_t *pbuf = pBuffer;

	pbuf +=  m_Info->dsp_width *  m_Info->start_y;

	for(int32_t i=0; i< m_Info->thickness; i++)
	{
		for(int32_t j= m_Info->start_x; j< m_Info->length +  m_Info->start_x ; j++)
		{
			pbuf[j] =  m_Info->color;
		}
		pbuf +=  m_Info->dsp_width;
	}
}

void VerticalLine(uint32_t *pBuffer, VLINE_INFO *m_Info)
{
	uint32_t *pbuf = pBuffer;
	int32_t x = m_Info->start_x;
	int32_t y = (m_Info->end_y-m_Info->start_y)/m_Info->offset_y;

	pbuf += m_Info->dsp_width * m_Info->start_y;

	for(int32_t i = 0; i<y; i++)
	{
		x += m_Info->offset_x;

		for(int32_t j=0; j < m_Info->offset_y; j++)
		{
			for(int32_t k=x; k<x + m_Info->thickness ; k++)
			{
				pbuf[k] = m_Info->color;
			}
			pbuf +=  m_Info->dsp_width;
		}
	}

}

static double now_ms(void)
{
	struct timespec res;
	clock_gettime(CLOCK_REALTIME, &res);
	return 1000.0*res.tv_sec + (double)res.tv_nsec/1e6;
}


//-----------------------------------------------------------------------------
static void PrintUsage( const char *appName )
{
	fprintf(stdout, "Usage : %s [options] \n", appName);
	fprintf(stdout, "  Options :\n");
	fprintf(stdout, "     -m module          : camera module                                    ex) -m 0\n");
	fprintf(stdout, "     -g gpio            : gpio idx for backgear detection                  ex) -g 71\n");
	fprintf(stdout, "     -b backgear        : backgear detection (enagble : 1, disable : 0)    ex) -b 0\n");
	fprintf(stdout, "     -c crtcID          : crtc ID                                          ex) -c 26\n");
	fprintf(stdout, "     -v planeID         : video layer for Cam                              ex) -v 27\n");
	fprintf(stdout, "     -p planeID         : rgb layer for PGL                                ex) -p 18\n");
	fprintf(stdout, "     -r widthxheight    : camera resolution                                ex) -r 960x480\n");
	fprintf(stdout, "     -d deinterlace engine : select deinterlace eginge\n");
	fprintf(stdout, "                             0 : non, 1:nexell, 2:thunder                  ex) -d 2\n");
	fprintf(stdout, "     -l log level       : change log level                                 ex) -l 2\n");
	fprintf(stdout, "     -s sensitivity     : motion sensitivity for deinterlace(thunder)      ex) -s 3\n");
	fprintf(stdout, "     -D x,y             : display position of camera data                  ex) -D 420,0\n");
	fprintf(stdout, "     -R widthxheight    : display size of camera data                      ex) -R 1080x720\n");
	fprintf(stdout, "     -P pgl_en          : parking guide line drawing (enable : 1. disalbe : 0)  ex) -P 1\n");
	fprintf(stdout, "     -L widthxheight    : display(LCD) size                                ex) -L 1920x720\n");
	fprintf(stdout,"\n");
}
//------------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                     BackGear CallBack function
//----------------------------------------------------------------------------
static int32_t backgear_status;
static int32_t change_backgear_status;
static void cbBackGearStatus( void *pObj, int32_t iStatus )
{
	(void)pObj;
	backgear_status  = iStatus ? NX_BACKGEAR_NOTDETECTED : NX_BACKGEAR_DETECTED;

	change_backgear_status = 1;

	// if(backgear_status)
	// {
	// 	printf("[QuickRearCam] : BackGear Detected\n");
	// }
	// else{
	// 	printf("[QuickRearCam] : BackGear Released\n");
	// }
}

//----------------------------------------------------------------------------
//                    Command CallBack function
//----------------------------------------------------------------------------
static int32_t received_stop_cmd;

static void cbQuickRearCamCommand( int32_t command )
{
	printf("cbQuickRearCamCommand : Command : %d , gear_status : %d\n", command, backgear_status);

	if(command == STOP)
	{
		received_stop_cmd = true;
	}
}
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
static char optstr[] = "hm:i:g:b:c:v:p:r:d:l:s:D:R:P:L:";

int32_t main( int argc, char **argv )
{
	double iStartTime = 0;
	double iEndTime = 0;

	void *m_pCmdHandle;

	HLINE_INFO m_HLine_Info[PGL_H_LINE_NUM];
	VLINE_INFO m_VLine_Info[PGL_V_LINE_NUM];

	int32_t m_bExitLoop = false;
	int32_t c;
	char *arg = NULL;
	uint32_t args = 0;
	int32_t iModule = 1;
	int32_t use_inter_cam = 1;
	int32_t gpioIdx = 71;  //navi-ref:163  Ruibao:43 convergence : 71
	int32_t backgear_enable = 1;
	int32_t crtcId = 26;
	int32_t videoPlaneId = 27;
	int32_t rgbPlaneId = 18;
	int32_t cam_width = 960;
	int32_t cam_height = 480;
	int32_t deinter_engine = THUNDER_DEINTERLACER;
	int32_t dbg_level;
	int32_t corr = 3;
	int32_t dp_x = 420;
	int32_t dp_y = 0;
	int32_t dp_w = 1080;
	int32_t dp_h = 720;
	int32_t pgl_en = 1;
	int32_t lcd_w = 1920;
	int32_t lcd_h = 720;
	int32_t m_iRearCamStatus = STATUS_STOP;

	bool m_bSetResolution = false;

	while ((c = getopt(argc, argv, optstr)) != -1) {
		args++;
		switch (c) {
		case 'm':   //module index
			arg = optarg;
			iModule = atoi(arg);
			NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] Module : %d\n", iModule );
			break;
		case 'i':   //interlace
			arg = optarg;
			use_inter_cam = atoi(arg);
			NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] Use interlace cam : %d\n", use_inter_cam );
			break;
		case 'g':  //gpio index for backgear
			arg = optarg;
			gpioIdx = atoi(arg);
			NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] BackGear gpio index : %d\n", gpioIdx );
			break;
		case 'b' :  //backgear detect enable/disable
			arg = optarg;
			backgear_enable = atoi(arg);
			break;
		case 'c' :  //crtc ID for display
			arg = optarg;
			crtcId = atoi(arg);
			NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] crtcId : %d\n", crtcId );
			break;
		case 'v' :  // video layer ID for rendering camera data
			arg = optarg;
			videoPlaneId = atoi(arg);
			NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] video planeId : %d\n", videoPlaneId );
			break;
		case 'p' : // rgb layer ID for rendering paring guide line
			arg = optarg;
			rgbPlaneId = atoi(arg);
			NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] rgb planeId : %d\n", rgbPlaneId );
			break;
		case 'r' :  // camera resolution
			arg = optarg;
			sscanf(arg, "%dx%d", &cam_width, &cam_height);
			m_bSetResolution = true;
			NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] camera resolution : %dx%d\n", cam_width, cam_height );
			break;
		case 'd' :  //deinterlace engine index
			arg = optarg;
			deinter_engine = atoi(arg);
			if(deinter_engine == NON_DEINTERLACER)
			{
				NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] deinter_engine : NON_DEINTERLACER \n");
			}
			else if(deinter_engine == NEXELL_DEINTERLACER)
			{
				NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] deinter_engine : NEXELL_DEINTERLACER \n");
			}
			else if(deinter_engine == THUNDER_DEINTERLACER)
			{
				NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] deinter_engine : THUNDER_DEINTERLACER \n");
			}
			else
			{
				NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] deinter_engine : NOT SUPPORT \n");
				deinter_engine = NON_DEINTERLACER;
			}
			break;
		case 'l':  //debug level
			dbg_level = atoi(optarg);
			if( (dbg_level >= NX_DBG_VBS) && (dbg_level <= NX_DBG_ERR) )
				NxChgFilterDebugLevel( atoi(optarg) );
			break;
		case 's':  //motion sensitivity for ThunderSoft deinterlace
			corr = atoi(optarg);
			break;
		case 'D':  //display start position(x, y) for video layer(camera data)
			arg = optarg;
			sscanf(arg, "%d,%d", &dp_x, &dp_y);
			NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] display position : (%d,%d)\n", dp_x, dp_y );
			break;
		case 'R':  //display resolution
			arg = optarg;
			sscanf(arg, "%dx%d", &dp_w, &dp_h);
			NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] display position : %dx%d\n", dp_w, dp_h );
			break;
		case 'P':  //Parking Guide Line Drawing
			pgl_en = atoi(optarg);
			break;
		case 'L':  //display resolution
			arg = optarg;
			sscanf(arg, "%dx%d", &lcd_w, &lcd_h);
			NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] display(LCD) size: %dx%d\n", lcd_w, lcd_h );
			break;
		case 'h':  //help
			PrintUsage("NxQuickRearCam");
			return 0;
			break;
		default:
			break;
		}
	}

	if(m_bSetResolution == false)
	{
		cam_width = 0;
		cam_height = 0;
	}

	if(backgear_enable == false)
		NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] DISABLE BackGear Detection\n");
	else
		NxDbgMsg( NX_DBG_INFO, "[QuickRearCam] ENABLE BackGear Detection\n");

#ifdef CHECK_TIME
	int64_t iCurTime = NxGetSystemTick();
	int64_t iPrvTime = iCurTime;

	NxDbgMsg(NX_DBG_INFO, "Camera Detection Start. ( %lld ms, %lld ms )\n", iCurTime, iCurTime - iPrvTime );
	iPrvTime = iCurTime;
#endif


	NX_REARCAM_INFO vip_info;       // camera info
	DISPLAY_INFO dsp_info;          // display info for rendering camera data
	DEINTERLACE_INFO deinter_info;  // deinterlace info
	NX_RGB_DRAW_INFO pgl_dsp_info;  //display info for rendering Parking guide line

	//information init
	memset( &vip_info, 0x00, sizeof(vip_info) );
	memset( &dsp_info, 0x00, sizeof(dsp_info) );
	memset( &pgl_dsp_info, 0x00, sizeof(pgl_dsp_info));

	//-------------------camera info setting---------------------------
	vip_info.iType			= CAM_TYPE_VIP;

	vip_info.iModule 		= iModule;

	vip_info.iSensor		= nx_sensor_subdev;
	vip_info.iClipper		= nx_clipper_subdev;
	vip_info.bUseMipi		= false;
	vip_info.bUseInterCam		= use_inter_cam;

	vip_info.iWidth			= cam_width;
	vip_info.iHeight		= cam_height;

	vip_info.iCropX			= 0;
	vip_info.iCropY 		= 0;
	vip_info.iCropWidth		= vip_info.iWidth;
	vip_info.iCropHeight	= vip_info.iHeight;
	vip_info.iOutWidth 		= vip_info.iWidth;
	vip_info.iOutHeight 	= vip_info.iHeight;
	//------------------------------------------------------------------

	//--------display info setting(for redering camera data)---------
	dsp_info.iPlaneId		= videoPlaneId;
	dsp_info.iCrtcId		= crtcId;
	dsp_info.uDrmFormat		= DRM_FORMAT_YUV420;
	dsp_info.iSrcWidth		= vip_info.iWidth;
	dsp_info.iSrcHeight		= vip_info.iHeight;
	dsp_info.iCropX			= 0;
	dsp_info.iCropY			= 0;
	dsp_info.iCropWidth		= vip_info.iWidth;
	dsp_info.iCropHeight	= vip_info.iHeight;
	dsp_info.iDspX			= dp_x;
	dsp_info.iDspY			= dp_y;
	dsp_info.iDspWidth 		= dp_w;
	dsp_info.iDspHeight		= dp_h;
	dsp_info.iPglEn			= pgl_en;

	// dsp_info.iPlaneId_PGL	= rgbPlaneId;
	// dsp_info.uDrmFormat_PGL = DRM_FORMAT_ARGB8888;
	//------------------------------------------------------------------

	//--------deinterlacer info setting(for deinterlace engine)---------
	deinter_info.iWidth		= cam_width;
	deinter_info.iHeight	= cam_height;


	if(vip_info.bUseInterCam == false)
	{
		deinter_info.iEngineSel = NON_DEINTERLACER;
	}else
	{
		deinter_info.iEngineSel = deinter_engine;
	}

	deinter_info.iCorr		= corr;
	//------------------------------------------------------------------

	//--------display info setting(for drawing parking guide dine)-------
	pgl_dsp_info.planeId 		= rgbPlaneId;
	pgl_dsp_info.crtcId 		= crtcId;
	pgl_dsp_info.m_iDspX 		= 0;
	pgl_dsp_info.m_iDspY		= 0;
	pgl_dsp_info.m_iDspWidth  	= lcd_w;
	pgl_dsp_info.m_iDspHeight 	= lcd_h;

	pgl_dsp_info.m_iNumBuffer   = 1;
	pgl_dsp_info.uDrmFormat 	= DRM_FORMAT_ARGB8888;
	//------------------------------------------------------------------
#ifdef CHECK_TIME
	iCurTime = NxGetSystemTick();
	NxDbgMsg(NX_DBG_INFO, "Camera Ready. ( %lld ms, %lld ms )\n", iCurTime, iCurTime - iPrvTime );
	iPrvTime = iCurTime;
#endif

	//-------- for backgear detecting------------------------
	backgear_status = NX_BACKGEAR_NOTDETECTED;
	change_backgear_status = 0;

	NX_RegisterBackGearEventCallBack(NULL,  cbBackGearStatus );

	if(backgear_enable == true)
		NX_StartBackGearDetectService(gpioIdx, 100);

	//------------------------------------------------------

	//-------- for stop command -----------------------------
	received_stop_cmd = false;
#ifndef ANDROID
	m_pCmdHandle = NX_GetCommandHandle ();
	NX_RegisterCommandEventCallBack (m_pCmdHandle, cbQuickRearCamCommand);
	NX_StartCommandService(m_pCmdHandle, STOP_COMMAND_CTRL_FILE_PATH);
#endif

	//-------------QuickRearCam init---------------------------
	if( 0 > NX_QuickRearCamInit( &vip_info, &dsp_info, &deinter_info ) )
		return -1;
	//--------------------------------------------------------

	//--------valiables for the example of drawing parking guid line----

	//declaration class for drawing parking guide line
	NX_CRGBDraw *m_pPGLDraw = new NX_CRGBDraw();

	NX_RGB_DRAW_MEMORY_INFO m_pglMemInfo;
	int32_t pgl_buf_idx = 0;
	uint32_t *pBuf = NULL;

	if(pgl_en == 1)
	{
		for(int32_t i=0; i<PGL_H_LINE_NUM; i++)
		{
			m_HLine_Info[i].dsp_width = pgl_dsp_info.m_iDspWidth;
			m_HLine_Info[i].start_x = (pgl_dsp_info.m_iDspWidth/2) - (dsp_info.iDspWidth /PGL_H_LINE_NUM)/2 - 90*i;
			m_HLine_Info[i].start_y = dsp_info.iDspHeight / 2 + 90*i;
			m_HLine_Info[i].length = (dsp_info.iDspWidth /PGL_H_LINE_NUM) + (90*2)*i;
			m_HLine_Info[i].thickness =  10;
			if(i == 0)
				m_HLine_Info[i].color = ARGB_COLOR_GREEN;
			else if(i == 1)
				m_HLine_Info[i].color = ARGB_COLOR_YELLOW;
			else
				m_HLine_Info[i].color = ARGB_COLOR_RED;

			m_VLine_Info[i*2].dsp_width = pgl_dsp_info.m_iDspWidth;
			m_VLine_Info[i*2].start_x = m_HLine_Info[i].start_x;
			m_VLine_Info[i*2].start_y = m_HLine_Info[i].start_y;
			m_VLine_Info[i*2].end_y = m_HLine_Info[i].start_y + 90;
			m_VLine_Info[i*2].thickness =  10;
			m_VLine_Info[i*2].offset_x = -1;
			m_VLine_Info[i*2].offset_y = 1;
			m_VLine_Info[i*2].color = m_HLine_Info[i].color;

			m_VLine_Info[i*2+1].dsp_width = pgl_dsp_info.m_iDspWidth;
			m_VLine_Info[i*2+1].start_x = m_HLine_Info[i].start_x + m_HLine_Info[i].length - m_HLine_Info[i].thickness;
			m_VLine_Info[i*2+1].start_y = m_HLine_Info[i].start_y;
			m_VLine_Info[i*2+1].end_y = m_HLine_Info[i].start_y + 90;
			m_VLine_Info[i*2+1].thickness =  10;
			m_VLine_Info[i*2+1].offset_x = 1;
			m_VLine_Info[i*2+1].offset_y = 1;
			m_VLine_Info[i*2+1].color = m_HLine_Info[i].color;

		}
	}


	bool	m_bPGLDraw = 0;
	//-------------------------------------------------------------------

	if(backgear_enable == true)	 //the case that is applied backgear detecting
	{
		while(!m_bExitLoop)
		{
			if(change_backgear_status == 1 && received_stop_cmd == false)
			{
				if(backgear_status == NX_BACKGEAR_DETECTED && m_iRearCamStatus == STATUS_STOP)
				{
					iStartTime = now_ms();
					//-------------QuickRearCam init---------------------------
					NX_QuickRearCamInit( &vip_info, &dsp_info, &deinter_info);
					//--------------------------------------------------------

					if(pgl_en == 1)
					{
						//-------RGB Draw Init(for drawing parking guide line)----
						m_pPGLDraw->Init(&pgl_dsp_info);

						//-------Fill color residual region-----------------------
						if(pgl_dsp_info.m_iDspWidth != dsp_info.iDspWidth)
						{
							m_pPGLDraw->FillColorBackGround(pgl_dsp_info.m_iDspWidth,
														(pgl_dsp_info.m_iDspWidth-dsp_info.iDspWidth)>>1,
														pgl_dsp_info.m_iDspHeight,
														0, 0, ARGB_COLOR_BLACK);
							m_pPGLDraw->FillColorBackGround(pgl_dsp_info.m_iDspWidth,
														(pgl_dsp_info.m_iDspWidth-dsp_info.iDspWidth)>>1,
														pgl_dsp_info.m_iDspHeight,
														(pgl_dsp_info.m_iDspWidth+dsp_info.iDspWidth)>>1, 0,
														ARGB_COLOR_BLACK);
						}
						//--------------------------------------------------------
					}
					//--------------------------------------------------------

					//-------------QuickRaerCam Start-------------------------
					NX_QuickRearCamStart();
					m_iRearCamStatus = STATUS_RUN;
					//--------------------------------------------------------

					m_bPGLDraw = true;   //setting flag for drawing parking guide line when detected backgear
					//--------------------------------------------------------
				}
				else
				{
					if(backgear_status == NX_BACKGEAR_NOTDETECTED && m_iRearCamStatus == STATUS_RUN)
					{
						m_bPGLDraw = false;	 //setting flag for not drawing parking guide line when released backgear

						//---------RGB drawing(parking guide line) Deinit
						if(pgl_en == 1)
						{
							m_pPGLDraw->Deinit();
						}
						//------------------------------------------------------

						//----------QuickRearCam Deinit-------------------------
						NX_QuickRearCamDeInit();
						//------------------------------------------------------
						usleep(50000);
						m_iRearCamStatus = STATUS_STOP;
					}
				}
				change_backgear_status = 0;
			}

			if(backgear_status == NX_BACKGEAR_NOTDETECTED && received_stop_cmd == true)
			{
				printf("----------------- stop\n");
				m_bExitLoop = true;
			}

			//---------------fill the buffer for rendering parking guide line----------
			// ARGB data and the way that fill the buffer, can be different with bellow.

			if(pgl_en == 1 && m_bPGLDraw == true )
			{
				iEndTime = now_ms();
				if((iEndTime - iStartTime) >= 300 ) // PGL drawing interval 300 ms
				{
					iStartTime = now_ms();

					//get buffer address for rgb data
					m_pPGLDraw->GetMemInfo(&m_pglMemInfo, pgl_buf_idx);
					//-------------------------------------------------
					pBuf = (uint32_t *)m_pglMemInfo.pBuffer;

					for(int32_t i=0; i<PGL_H_LINE_NUM; i++)
					{
						HorizontalLine(pBuf, &m_HLine_Info[i]);
						VerticalLine(pBuf, &m_VLine_Info[i*2]);
						VerticalLine(pBuf, &m_VLine_Info[i*2+1]);
					}
					//------------------------------------------------------------------

					//------------ARGB data rendering-----------------------------------
					m_pPGLDraw->Render(pgl_buf_idx);
					//------------------------------------------------------------------

					//for buffer control for an example of drawing parking guide line
					pgl_buf_idx++;
					if(pgl_buf_idx >= pgl_dsp_info.m_iNumBuffer)
					{
						pgl_buf_idx = 0;
					}

					m_bPGLDraw = false;		//Just once drawing PGL when detected backgear.
									//If you want to draw PGL at rear-time, this line has to be diabled.
				}
			}
			usleep(50000);
		}
	}else  //the case that isn't applied backgear detecting
	{
		//-----------------QuickRearCam Init---------------------------
		NX_QuickRearCamInit( &vip_info, &dsp_info, &deinter_info );
		//-------------------------------------------------------------

		if(pgl_en == 1)
		{
			//--------------- RGB Draw Init(for parking guide line)--------
			m_pPGLDraw->Init(&pgl_dsp_info);
			//-------------------------------------------------------------

			//-------Fill color residual region-----------------------
			if(pgl_dsp_info.m_iDspWidth != dsp_info.iDspWidth)
			{
				m_pPGLDraw->FillColorBackGround(pgl_dsp_info.m_iDspWidth,
											(pgl_dsp_info.m_iDspWidth-dsp_info.iDspWidth)>>1,
											pgl_dsp_info.m_iDspHeight,
											0, 0, ARGB_COLOR_BLACK);
				m_pPGLDraw->FillColorBackGround(pgl_dsp_info.m_iDspWidth,
											(pgl_dsp_info.m_iDspWidth-dsp_info.iDspWidth)>>1,
											pgl_dsp_info.m_iDspHeight,
											(pgl_dsp_info.m_iDspWidth+dsp_info.iDspWidth)>>1, 0,
											ARGB_COLOR_BLACK);
			}
		}

		//----------------QuickRearCam Start---------------------------
		NX_QuickRearCamStart();
		//-------------------------------------------------------------


		//----------------Drawing parking guide line-------------------
		while(1)
		{
			if(pgl_en == 1)
			{
				//get buffer address for rgb data
				m_pPGLDraw->GetMemInfo(&m_pglMemInfo, pgl_buf_idx);
				//--------------------------------------------------------

				//---------------fill the buffer for rendering parking guide line----------
				// ARGB data and the way that fill the buffer, can be different way with bellow.
				pBuf = (uint32_t *)m_pglMemInfo.pBuffer;

				for(int32_t i=0; i<PGL_H_LINE_NUM; i++)
				{
					HorizontalLine(pBuf, &m_HLine_Info[i]);
					VerticalLine(pBuf, &m_VLine_Info[i*2]);
					VerticalLine(pBuf, &m_VLine_Info[i*2+1]);
				}

				//-----------------------------------------------------------------------

				//------------ARGB data rendering-----------------------------------
				m_pPGLDraw->Render(pgl_buf_idx);
				//------------------------------------------------------------------

				pgl_buf_idx++;
				if(pgl_buf_idx >= pgl_dsp_info.m_iNumBuffer)
				{
					pgl_buf_idx = 0;
				}
			}
			usleep(300000);   //for drawing interval
		}
		//-------------------------------------------------------------
	}

	//------------backgear detection service stop---------------------
	if(backgear_enable == true)
		NX_StopBackGearDetectService();
	//---------------------------------------------------------------

	//-----------QuickRearCam Deinit---------------------------------
	NX_QuickRearCamDeInit();
	//---------------------------------------------------------------

	//-----------RGB Draw Deinit-------------------------------------
	m_pPGLDraw->Deinit();
	//---------------------------------------------------------------

	//-----------Stop Command Service -------------------------------
#ifndef ANDROID
	NX_StopCommandService(m_pCmdHandle, STOP_COMMAND_CTRL_FILE_PATH);
#endif

	return 0;
}







