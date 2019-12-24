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

#include "NX_CRearCamManager.h"

#include <nx-v4l2.h>
#include <drm_mode.h>
#include <drm_fourcc.h>
#define virtual vir
#include <xf86drm.h>
#include <xf86drmMode.h>
#undef virtual

#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_CRearCamManager]"
#include <NX_DbgMsg.h>

#define VER_MAJOR   0
#define VER_MINOR   1
#define VER_REVISION    0
#define VER_RESERVATION 0

//------------------------------------------------------------------------------
NX_CRearCamManager::NX_CRearCamManager()
	: NotifyCallbackFunc	( NULL )
	, mRearCamStatus	(0)
	, m_pEventNotifier	( NULL )
	, m_pV4l2VipFilter	( NULL )
	, m_pDeinterlaceFilter	( NULL )
	, m_pVideoRenderFilter	( NULL )
	, m_MemDevFd		(-1)
	, m_CamDevFd		(-1)
	, m_DPDevFd		(-1)
#ifdef ANDROID_SURF_RENDERING
	, m_pAndroidRenderer	( NULL )
#endif
{
}

//------------------------------------------------------------------------------
NX_CRearCamManager::~NX_CRearCamManager()
{
	Stop();
	Deinit();
}

//------------------------------------------------------------------------------
int32_t NX_CRearCamManager::InitRearCamManager(NX_REARCAM_INFO *pInfo , DISPLAY_INFO *pDspInfo, DEINTERLACE_INFO *pDeinterInfo)
{
	memset( &videoInfo, 0x00, sizeof(videoInfo) );
	memset( &dspInfo, 0x00, sizeof(dspInfo));
	if( pInfo->iType == CAM_TYPE_VIP )
	{
		videoInfo.iMediaType		= NX_TYPE_VIDEO;
		videoInfo.iContainerType	= NX_CONTAINER_MP4;
		videoInfo.iModule			= pInfo->iModule;
		videoInfo.iSensorId			= pInfo->iSensor;
		videoInfo.bInterlace		= pInfo->bUseInterCam;
		videoInfo.iWidth			= pInfo->iWidth;
		videoInfo.iHeight			= pInfo->iHeight;
		videoInfo.iFpsNum			= 30;
		videoInfo.iFpsDen			= 1;
		videoInfo.iNumPlane			= 1;
		videoInfo.iCodec			= NX_CODEC_H264;
		videoInfo.iCropX			= pInfo->iCropX;
		videoInfo.iCropY			= pInfo->iCropY;
		videoInfo.iCropWidth		= pInfo->iCropWidth;
		videoInfo.iCropHeight		= pInfo->iCropHeight;
		videoInfo.iOutWidth			= pInfo->iOutWidth;
		videoInfo.iOutHeight		= pInfo->iOutHeight;
	}

	dspInfo.connectorId 		= pDspInfo->iConnectorId;
	dspInfo.planeId				= pDspInfo->iPlaneId;
	dspInfo.ctrlId				= pDspInfo->iCrtcId;
	dspInfo.width				= pDspInfo->iSrcWidth;
	dspInfo.height				= pDspInfo->iSrcHeight;
	dspInfo.stride				= 0;
	dspInfo.drmFormat			= pDspInfo->uDrmFormat;
	dspInfo.numPlane				= 1;//MEM_NUM_PLANE;
	dspInfo.dspSrcRect.left		= pDspInfo->iCropX;
	dspInfo.dspSrcRect.top		= pDspInfo->iCropY;
	dspInfo.dspSrcRect.right		= pDspInfo->iCropX + pDspInfo->iCropWidth;
	dspInfo.dspSrcRect.bottom	= pDspInfo->iCropY + pDspInfo->iCropHeight;
	dspInfo.dspDstRect.left		= pDspInfo->iDspX;
	dspInfo.dspDstRect.top		= pDspInfo->iDspY;
	dspInfo.dspDstRect.right		= pDspInfo->iDspWidth;
	dspInfo.dspDstRect.bottom		= pDspInfo->iDspHeight;
	dspInfo.pglEnable				= pDspInfo->iPglEn;
	dspInfo.lcd_width			= pDspInfo->iLCDWidth;
	dspInfo.lcd_height			= pDspInfo->iLCDHeight;
	dspInfo.setCrtc				= pDspInfo->bSetCrtc;


	dspInfo.pNativeWindow		= pDspInfo->m_pNativeWindow;


	deinterInfo.width 			= pDeinterInfo->iWidth;
	deinterInfo.height			= pDeinterInfo->iHeight;
	deinterInfo.engineSel		= pDeinterInfo->iEngineSel;
	deinterInfo.corr			= pDeinterInfo->iCorr;

	//get video resolution
	if(videoInfo.iWidth == 0 || videoInfo.iHeight == 0)
	{
		m_pV4l2VipFilter->GetResolution(nx_clipper_video, videoInfo.iModule, &videoInfo.iWidth, &videoInfo.iHeight);
		NxDbgMsg( NX_DBG_ERR, "%s()--frame_width : %d, frame_height : %d\n", __FUNCTION__ , videoInfo.iWidth, videoInfo.iHeight );

		videoInfo.iCropWidth 		= videoInfo.iWidth;
		videoInfo.iCropHeight 		= videoInfo.iHeight;
		videoInfo.iOutWidth			= videoInfo.iWidth;
		videoInfo.iOutHeight 		= videoInfo.iHeight;
		dspInfo.width 				= videoInfo.iWidth;
		dspInfo.height 				= videoInfo.iHeight;
		dspInfo.dspSrcRect.right 	= pDspInfo->iCropX + videoInfo.iWidth;
		dspInfo.dspSrcRect.bottom 	= pDspInfo->iCropY + videoInfo.iHeight;
		deinterInfo.width 			= videoInfo.iWidth;
		deinterInfo.height 			= videoInfo.iHeight;
	}

	//device open--------------------------------------------------------------------------
#ifdef USE_ION_ALLOCATOR
	m_MemDevFd = open( "/dev/ion", O_RDWR );
#else
	m_MemDevFd = drmOpen( "nexell", NULL );
#endif
	m_CamDevFd = nx_v4l2_open_device(nx_clipper_video, videoInfo.iModule);
	m_DPDevFd = drmOpen( "nexell", NULL );

	printf("m_MemDevFd / m_CamDevFd / m_DPDevFd : %d / %d / %d \n", m_MemDevFd, m_CamDevFd, m_DPDevFd);
	//--------------------------------------------------------------------------------------

	NX_CAutoLock lock( &m_hLock );
	//
	//	Create Instance
	//
	m_pRefClock			= new NX_CRefClock();
	m_pEventNotifier	= new NX_CEventNotifier();

#ifdef ANDROID_SURF_RENDERING
	if(pDspInfo->m_pNativeWindow)
	{
		m_pAndroidRenderer = new NX_CAndroidRenderer((ANativeWindow*) pDspInfo->m_pNativeWindow);
	}
#endif
	m_pVideoRenderFilter= new NX_CVideoRenderFilter();

	m_pV4l2VipFilter		= new NX_CV4l2VipFilter();


	if( deinterInfo.engineSel != NON_DEINTERLACER)
	{
		m_pDeinterlaceFilter 		= new NX_CDeinterlaceFilter();
	}


	if( m_pV4l2VipFilter		) 	m_pV4l2VipFilter		->SetRefClock( m_pRefClock );


	//
	//	Set Notifier
	//
	if( m_pV4l2VipFilter		) m_pV4l2VipFilter		->SetEventNotifier( m_pEventNotifier );


	//get video resolution
	if(videoInfo.iWidth == 0 || videoInfo.iHeight == 0)
	{
		m_pV4l2VipFilter->GetResolution(nx_clipper_video, videoInfo.iModule, &videoInfo.iWidth, &videoInfo.iHeight);
		NxDbgMsg( NX_DBG_ERR, "%s()--frame_width : %d, frame_height : %d\n", __FUNCTION__ , videoInfo.iWidth, videoInfo.iHeight );

		videoInfo.iCropWidth 		= videoInfo.iWidth;
		videoInfo.iCropHeight 		= videoInfo.iHeight;
		videoInfo.iOutWidth			= videoInfo.iWidth;
		videoInfo.iOutHeight 		= videoInfo.iHeight;
		dspInfo.width 				= videoInfo.iWidth;
		dspInfo.height 				= videoInfo.iHeight;
		dspInfo.dspSrcRect.right 	= pDspInfo->iCropX + videoInfo.iWidth;
		dspInfo.dspSrcRect.bottom 	= pDspInfo->iCropY + videoInfo.iHeight;
		deinterInfo.width 			= videoInfo.iWidth;
		deinterInfo.height 			= videoInfo.iHeight;
	}


	//
	//	Set Configuration
	//
#ifdef ANDROID_SURF_RENDERING
	if(pDspInfo->m_pNativeWindow == NULL)
	{
		if( m_pV4l2VipFilter		) m_pV4l2VipFilter		->SetConfig( &videoInfo, NULL);
		if( m_pVideoRenderFilter 	) m_pVideoRenderFilter  ->SetConfig( &dspInfo, NULL);
		if( m_pDeinterlaceFilter	) m_pDeinterlaceFilter  ->SetConfig( &deinterInfo, NULL);
	}

	else
	{	if(deinterInfo.engineSel == NON_DEINTERLACER)
		{
			if( m_pV4l2VipFilter		) m_pV4l2VipFilter		->SetConfig( &videoInfo, m_pAndroidRenderer);
			if( m_pVideoRenderFilter 	) m_pVideoRenderFilter  ->SetConfig( &dspInfo, m_pAndroidRenderer);
			if( m_pDeinterlaceFilter	) m_pDeinterlaceFilter  ->SetConfig( &deinterInfo, NULL);
		}else
		{
			if( m_pV4l2VipFilter		) m_pV4l2VipFilter		->SetConfig( &videoInfo, NULL);
			if( m_pVideoRenderFilter 	) m_pVideoRenderFilter  ->SetConfig( &dspInfo, m_pAndroidRenderer);
			if( m_pDeinterlaceFilter	) m_pDeinterlaceFilter  ->SetConfig( &deinterInfo, m_pAndroidRenderer);
		}
	}
#else
	if( m_pV4l2VipFilter		) m_pV4l2VipFilter		->SetConfig( &videoInfo);
	if( m_pVideoRenderFilter 	) m_pVideoRenderFilter  ->SetConfig( &dspInfo);
	if( m_pDeinterlaceFilter	) m_pDeinterlaceFilter  ->SetConfig( &deinterInfo);

#endif
	//
	//	Pin Negotiate
	//
	NX_CBasePin	*pInputPin, *pOutputPin;


	if( m_pV4l2VipFilter && m_pDeinterlaceFilter && m_pVideoRenderFilter )
	{
		pOutputPin	= m_pV4l2VipFilter->FindPin( NX_PIN_OUTPUT, 0 );
		pInputPin 	= m_pDeinterlaceFilter->FindPin (NX_PIN_INPUT, 0);
		if( 0 > ((NX_CBaseInputPin*)pInputPin)->PinNegotiation( (NX_CBaseOutputPin*)pOutputPin ) ) goto Error;

		pOutputPin 	= m_pDeinterlaceFilter->FindPin(NX_PIN_OUTPUT, 0);
		pInputPin	= m_pVideoRenderFilter->FindPin( NX_PIN_INPUT, 0 );
		if( 0 > ((NX_CBaseInputPin*)pInputPin)->PinNegotiation( (NX_CBaseOutputPin*)pOutputPin ) ) goto Error;
	}
	else if(deinterInfo.engineSel == NON_DEINTERLACER && m_pV4l2VipFilter && m_pVideoRenderFilter)  //bypass
	{
		pOutputPin	= m_pV4l2VipFilter->FindPin( NX_PIN_OUTPUT, 0 );
		pInputPin	= m_pVideoRenderFilter->FindPin( NX_PIN_INPUT, 0 );
		if( 0 > ((NX_CBaseInputPin*)pInputPin)->PinNegotiation( (NX_CBaseOutputPin*)pOutputPin ) ) goto Error;
	}

	if(m_pV4l2VipFilter	) m_pV4l2VipFilter	->SetDeviceFD(m_CamDevFd, m_MemDevFd);
	if(m_pDeinterlaceFilter	) m_pDeinterlaceFilter	->SetDeviceFD(m_MemDevFd);
	if(m_pVideoRenderFilter	) m_pVideoRenderFilter	->SetDeviceFD(m_DPDevFd);

	mRearCamStatus = REAR_CAM_STATUS_STOP;


	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;

Error:
	if( m_pRefClock				) { delete m_pRefClock;				m_pRefClock				= NULL;	}
	if( m_pV4l2VipFilter		) {	delete m_pV4l2VipFilter;		m_pV4l2VipFilter		= NULL; }
	if( m_pDeinterlaceFilter    ) { delete m_pDeinterlaceFilter;    m_pDeinterlaceFilter    = NULL; }
	if( m_pVideoRenderFilter	) {	delete m_pVideoRenderFilter;	m_pVideoRenderFilter	= NULL; }
#ifdef ANDROID_SURF_RENDERING
	if(m_pAndroidRenderer		) {	delete m_pAndroidRenderer;		m_pAndroidRenderer		= NULL; }
#endif
	NxDbgMsg( NX_DBG_ERR, "%s()--NxQuickRearCam Manager Init Fail\n", __FUNCTION__ );
	return -1;

}

//------------------------------------------------------------------------------
int32_t NX_CRearCamManager::Init()
{
	NxDbgMsg( NX_DBG_INFO, "%s()++\n", __FUNCTION__ );

	int32_t ret = 0;

	mRearCamStatus = REAR_CAM_STATUS_STOP;

	if(m_pV4l2VipFilter)
	{
		ret = m_pV4l2VipFilter->Init();
		if(ret < 0)
		{
			printf("V4l2VipFilter Init Fail\n");
			return -1;
		}
	}

	if(m_pDeinterlaceFilter)
	{
		ret = m_pDeinterlaceFilter->Init();
		if(ret < 0)
		{
			printf("DeinterlaceFilter Init Fail\n");
			return -1;
		}
	}

	if(m_pVideoRenderFilter)
	{
		ret = m_pVideoRenderFilter->Init();
		if(ret < 0)
		{
			printf("VideoRenderFilter Init Fail\n");
			return -1;
		}
	}

	NxDbgMsg( NX_DBG_INFO, "%s()--\n", __FUNCTION__ );

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CRearCamManager::Deinit( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	if(m_pRefClock)
	{
		delete m_pRefClock;
		m_pRefClock	= NULL;
	}

	if(m_pV4l2VipFilter)
	{
		m_pV4l2VipFilter->Deinit();
		delete m_pV4l2VipFilter;
		m_pV4l2VipFilter = NULL;
	}

	if(m_pDeinterlaceFilter)
	{
		m_pDeinterlaceFilter->Deinit();
		delete m_pDeinterlaceFilter;
		m_pDeinterlaceFilter = NULL;
	}

	if(m_pVideoRenderFilter)
	{
		m_pVideoRenderFilter->Deinit();
		delete m_pDeinterlaceFilter;
		m_pDeinterlaceFilter = NULL;
	}

#ifdef ANDROID_SURF_RENDERING
	if(m_pAndroidRenderer		) {	delete m_pAndroidRenderer;		m_pAndroidRenderer		= NULL; }
#endif

	mRearCamStatus = REAR_CAM_STATUS_STOP;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CRearCamManager::Start( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	int32_t ret = 0;
	NX_CAutoLock lock( &m_hLock );

	if( m_pVideoRenderFilter)
	{
		if(m_pVideoRenderFilter->Run() < 0)
		{
			NxDbgMsg( NX_DBG_VBS, "%s()--RenderFilter Run Fail\n", __FUNCTION__ );
			Stop();
			Deinit();
			return -1;
		}
	}

	if( m_pDeinterlaceFilter )
	{
		if(m_pDeinterlaceFilter->Run() < 0)
		{
			NxDbgMsg( NX_DBG_VBS, "%s()--DeinterlaceFilter Run Fail\n", __FUNCTION__ );
			Stop();
			Deinit();
			return -1;
		}
	}

	if( m_pV4l2VipFilter )
	{
		if(m_pV4l2VipFilter->Run() < 0)
		{
			NxDbgMsg( NX_DBG_VBS, "%s()--DeinterlaceFilter Run Fail\n", __FUNCTION__ );
			Stop();
			Deinit();
			return -1;
		}
	}

	mRearCamStatus = REAR_CAM_STATUS_RUNNING;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return ret;
}

//------------------------------------------------------------------------------
int32_t NX_CRearCamManager::Stop( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	if( m_pVideoRenderFilter	) m_pVideoRenderFilter	->Stop();
	if( m_pDeinterlaceFilter    ) m_pDeinterlaceFilter  ->Stop();
	if( m_pV4l2VipFilter		) m_pV4l2VipFilter		->Stop();

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CRearCamManager::Pause( int32_t mbPause )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	if( m_pVideoRenderFilter	) m_pVideoRenderFilter	->Pause(mbPause);
	if( m_pDeinterlaceFilter    ) m_pDeinterlaceFilter  ->Pause(mbPause);
	if( m_pV4l2VipFilter		) m_pV4l2VipFilter		->Pause(mbPause);

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}


//------------------------------------------------------------------------------
int32_t NX_CRearCamManager::RegNotifyCallback( void (*cbFunc)(uint32_t, void*, uint32_t) )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	NotifyCallbackFunc = cbFunc;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CRearCamManager::ChangeDebugLevel( int32_t iLevel )
{
	NX_CAutoLock lock( &m_hLock );

	int32_t iDebugLevel = NX_DBG_INFO;

	if( 0 == iLevel ) 		iDebugLevel = NX_DBG_DISABLE;
	else if( 1 == iLevel )	iDebugLevel = NX_DBG_ERR;
	else if( 2 == iLevel )	iDebugLevel = NX_DBG_WARN;
	else if( 3 == iLevel )	iDebugLevel = NX_DBG_INFO;
	else if( 4 == iLevel )	iDebugLevel = NX_DBG_DEBUG;
	else if( 5 == iLevel )	iDebugLevel = NX_DBG_VBS;

	NxChgFilterDebugLevel( iDebugLevel );
	return 0;
}

//------------------------------------------------------------------------------
void NX_CRearCamManager::ProcessEvent( uint32_t iEventCode, void *pData, uint32_t iDataSize )
{
	if( NotifyCallbackFunc && (iEventCode < NOTIFY_INTERNAL) ) {
		NotifyCallbackFunc( iEventCode, pData, iDataSize );
		return;
	}

	switch( iEventCode )
	{
		case NOTIFY_CAPTURE_DONE:
		{
			NxDbgMsg( NX_DBG_INFO, "Capture Done. (%s)\n", (char*)pData );
			break;
		}

		default:
			break;
	}
}

//------------------------------------------------------------------------------
int32_t NX_CRearCamManager::GetStatus()
{
	NxDbgMsg( NX_DBG_INFO, "%s() ++++ : status : %d\n", __FUNCTION__, mRearCamStatus );
	return mRearCamStatus;
}

//------------------------------------------------------------------------------
int32_t NX_CRearCamManager::SetDisplayPosition(int32_t x, int32_t y, int32_t w, int32_t h)
{
	NxDbgMsg( NX_DBG_INFO, "%s() ++++ \n", __FUNCTION__);

	if(m_pVideoRenderFilter != NULL)
		m_pVideoRenderFilter->SetPosition(x, y, w, h);

	return 0;
}

// //------------------------------------------------------------------------------
void NX_QuiclRearCamReleaseHandle( void* hRearCam )
{
	NX_CRearCamManager *pstRearCamManager = NULL;
	pstRearCamManager = (NX_CRearCamManager*) hRearCam;

	drmClose(pstRearCamManager->m_DPDevFd);
	close(pstRearCamManager->m_CamDevFd);
	close(pstRearCamManager->m_MemDevFd);

	delete pstRearCamManager;
	pstRearCamManager = NULL;
}

//------------------------------------------------------------------------------
void *NX_QuickRearCamGetHandle(NX_REARCAM_INFO *p_VipInfo, DISPLAY_INFO* p_dspInfo, DEINTERLACE_INFO *p_deinterInfo)
{
	int32_t ret = 0;
	NX_CRearCamManager *pstRearCamManager = NULL;
	pstRearCamManager = new NX_CRearCamManager();

	ret = pstRearCamManager->InitRearCamManager(p_VipInfo, p_dspInfo, p_deinterInfo);
	if( 0 > ret )
		return NULL;

	return (void*)pstRearCamManager;
}

//------------------------------------------------------------------------------
int32_t NX_QuickRearCamInit(void* hRearCam)
{
	NxDbgMsg( NX_DBG_INFO, "%s() ++++ \n", __FUNCTION__ );
	NX_CRearCamManager *pstRearCamManager = NULL;
	int32_t ret = 0;

	pstRearCamManager = (NX_CRearCamManager*) hRearCam;

	ret = pstRearCamManager->Init();

	NxDbgMsg( NX_DBG_INFO, "%s() ----- \n", __FUNCTION__ );

	return ret;
}

//------------------------------------------------------------------------------
int32_t NX_QuickRearCamStop(void* hRearCam)
{
	NxDbgMsg( NX_DBG_INFO, "%s() ++++ \n", __FUNCTION__ );
	NX_CRearCamManager *pstRearCamManager = NULL;

	pstRearCamManager = (NX_CRearCamManager*) hRearCam;

	pstRearCamManager->Stop();

	NxDbgMsg( NX_DBG_INFO, "%s() ----- \n", __FUNCTION__ );

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_QuickRearCamDeInit(void* hRearCam)
{
	NxDbgMsg( NX_DBG_INFO, "%s() ++++ \n", __FUNCTION__ );
	NX_CRearCamManager *pstRearCamManager = NULL;

	pstRearCamManager = (NX_CRearCamManager*) hRearCam;

	pstRearCamManager->Deinit();

	NxDbgMsg( NX_DBG_INFO, "%s() ----- \n", __FUNCTION__ );

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_QuickRearCamStart(void* hRearCam)
{
	NxDbgMsg( NX_DBG_INFO, "%s() ++++ \n", __FUNCTION__ );
	NX_CRearCamManager *pstRearCamManager;
	int32_t ret = 0;

	pstRearCamManager = (NX_CRearCamManager*) hRearCam;

	ret = pstRearCamManager->Start();

	return ret;
}

//------------------------------------------------------------------------------
int32_t NX_QuickRearCamPause(void* hRearCam, int32_t m_bPause)
{
	NX_CRearCamManager *pstRearCamManager;
	int32_t ret = 0;

	pstRearCamManager = (NX_CRearCamManager*) hRearCam;

	ret = pstRearCamManager->Pause(m_bPause);

	return ret;
}

//------------------------------------------------------------------------------
int32_t NX_QuickRearCamGetStatus(void* hRearCam)
{
	NxDbgMsg( NX_DBG_INFO, "%s() ++++ \n", __FUNCTION__ );
	NX_CRearCamManager *pstRearCamManager;
	int32_t ret = 0;

	pstRearCamManager = (NX_CRearCamManager*) hRearCam;

	ret = pstRearCamManager->GetStatus();

	return ret;
}

//---------------------------------------------------------------------------
int32_t NX_QuickRearCamSetDisplayPosition(void* hRearCam, int32_t x, int32_t y, int32_t w, int32_t h)
{
	NxDbgMsg( NX_DBG_INFO, "%s() ++++ \n", __FUNCTION__ );
	NX_CRearCamManager *pstRearCamManager;
	int32_t ret = 0;

	pstRearCamManager = (NX_CRearCamManager*) hRearCam;

	ret = pstRearCamManager->SetDisplayPosition(x, y, w, h);

	return ret;
}

//---------------------------------------------------------------------------
int32_t NX_QuickRearCamGetMemDevFd(void* hRearCam)
{
	NX_CRearCamManager *pstRearCamManager;
	pstRearCamManager = (NX_CRearCamManager*) hRearCam;

	return (pstRearCamManager->m_MemDevFd);
}

//---------------------------------------------------------------------------
int32_t NX_QuickRearCamGetCamDevFd(void* hRearCam)
{
	NX_CRearCamManager *pstRearCamManager;
	pstRearCamManager = (NX_CRearCamManager*) hRearCam;

	return (pstRearCamManager->m_CamDevFd);
}

//---------------------------------------------------------------------------
int32_t NX_QuickRearCamGetDPDevFd(void* hRearCam)
{
	NX_CRearCamManager *pstRearCamManager;
	pstRearCamManager = (NX_CRearCamManager*) hRearCam;

	return (pstRearCamManager->m_DPDevFd);
}

//---------------------------------------------------------------------------
int32_t NX_QuickRearCamGetVersion()
{
	NxDbgMsg( NX_DBG_INFO, "%s() --- NxQuickRearCam Version : %d.%d.%d.%d", __FUNCTION__, VER_MAJOR, VER_MINOR, VER_REVISION, VER_RESERVATION);

	return ((VER_MAJOR << 24) | (VER_MINOR << 16) | (VER_REVISION << 8) | (VER_RESERVATION));
}

