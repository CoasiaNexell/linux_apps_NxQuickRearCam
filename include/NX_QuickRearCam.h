//------------------------------------------------------------------------------
//
//	Copyright (C) 2018 Nexell Co. All Rights Reserved
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

#ifndef __NX_QUICK_REAR_CAM_H__
#define __NX_QUICK_REAR_CAM_H__

#ifdef __cplusplus

#include <stdint.h>

// #ifdef ANDROID
// #include <android/native_window.h>
// #include <android/native_window_jni.h>
// #endif

typedef struct _NX_REARCAM_INFO {
	int32_t iType;
	int32_t iModule;
	int32_t iSensor;
	int32_t iClipper;
	int32_t bUseMipi;
	int32_t bUseInterCam;

	int32_t iFpsNum;
	int32_t iFpsDen;
	int32_t iNumPlane;

	int32_t iWidth;
	int32_t iHeight;

	int32_t		iCropX;			//	Cliper x
	int32_t		iCropY;			//	Cliper y
	int32_t		iCropWidth;		//	Cliper width
	int32_t		iCropHeight;	//	Cliper height

	int32_t		iOutWidth;		//	Decimator width
	int32_t		iOutHeight;		//	Decimator height


} NX_REARCAM_INFO;


typedef struct tagDISPLAY_INFO{
	uint32_t    iConnectorId;	//  DRM Connector ID
	int32_t		iPlaneId;       //  DRM Plane ID
	int32_t		iCrtcId;        //  DRM CRTC ID
	uint32_t	uDrmFormat;		//	DRM Data Format
	int32_t		iSrcWidth;      //  Input Image's Width
	int32_t		iSrcHeight;     //  Input Image's Height
	int32_t		iCropX;         //  Input Source Position
	int32_t		iCropY;
	int32_t		iCropWidth;
	int32_t		iCropHeight;
	int32_t		iDspX;          //  Display Position
	int32_t		iDspY;
	int32_t		iDspWidth;
	int32_t		iDspHeight;
	int32_t		iLCDWidth;
	int32_t		iLCDHeight;
	int32_t		bSetCrtc;

	int32_t 	iPlaneId_PGL;   // parking line rendering plane ID
	int32_t		uDrmFormat_PGL;  // parking line drm data format

	int32_t 	iPglEn;			//PGL Drawing Enable

	void*    m_pNativeWindow;

}DISPLAY_INFO;


typedef struct tagDEINTERLACE_INFO{
	int32_t 	iWidth;
	int32_t		iHeight;
	int32_t		iEngineSel;
	int32_t		iCorr;
}DEINTERLACE_INFO;

typedef struct NX_DISPLAY_INFO	*NX_DISPLAY_HANDLE;

enum {
	CAM_TYPE_NONE = 0,
	CAM_TYPE_VIP,
};

enum {
	STATUS_STOP = 0,
	STATUS_RUN,
	STATUS_MAX
};

enum deinter_engine_sel {
	NON_DEINTERLACER = 0,
	NEXELL_DEINTERLACER,
	THUNDER_DEINTERLACER,
	DEINTERLACER_MAX
};


int32_t NX_QuickRearCamInit(NX_REARCAM_INFO *p_vipInfo, DISPLAY_INFO* p_dspInfo, DEINTERLACE_INFO* p_deinterInfo);
int32_t NX_QuickRearCamDeInit();
int32_t NX_QuickRearCamStart();
int32_t NX_QuickRearCamPause(int32_t m_bPause);
int32_t NX_QuickRearCamGetStatus();
int32_t NX_QuickRearCamGetVersion();
int32_t NX_QuickRearCamSetDisplayPosition(int32_t x, int32_t y, int32_t w, int32_t h);



#endif	// __cplusplus

#endif	// __NX_QUICK_REAR_CAM_H__
