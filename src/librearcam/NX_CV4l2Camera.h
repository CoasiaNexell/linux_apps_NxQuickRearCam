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

#ifndef __NX_CV4L2CAMERA_H__
#define __NX_CV4L2CAMERA_H__

#ifdef __cplusplus

#include <stdint.h>
#include <pthread.h>

#include <nx_video_api.h>
#include <nx-v4l2.h>
//#include <nx-drm-allocator.h>

#ifndef NX_MAX_PLANES
#define	NX_MAX_PLANES	4
#endif

enum
{
	NX_VIP_NOP		= -1,
};

typedef struct _NX_VIP_INFO
{
	int32_t		iModule;
	int32_t		iSensorId;
	int32_t		iBusFormat;
	int32_t		bInterlace;

	int32_t		iWidth;			//	Camera Input Width
	int32_t		iHeight;		//	Camera Input Height

	int32_t		iFpsNum;		//	Frame per seconds's Numerate value
	int32_t		iFpsDen;		//	Frame per seconds's Denominate value

	int32_t 	iNumPlane;		//	Input image's plane number

	int32_t		iCropX;			//	Cliper x
	int32_t		iCropY;			//	Cliper y
	int32_t		iCropWidth;		//	Cliper width
	int32_t		iCropHeight;	//	Cliper height

	int32_t		iOutWidth;		//	Decimator width
	int32_t		iOutHeight;		//	Decimator height
} NX_VIP_INFO;

class NX_CV4l2Camera
{
public:
	NX_CV4l2Camera();
	~NX_CV4l2Camera();

public:
	int32_t	Init( NX_VIP_INFO *pInfo );
	void 	Deinit( void );
	int32_t	QueueBuffer( int32_t bufferIndex );
	int32_t DequeueBuffer( int32_t *pBufferIndex );
	int32_t SetVideoMemory( NX_VID_MEMORY_INFO *pVidMem );
	int32_t AddVideoMemory( NX_VID_MEMORY_INFO *pVidMem );

	int32_t V4l2SensorGetStatus();
	int32_t	V4l2SensorSetMux(int value);

private:
	int32_t	V4l2CameraInit();
	int32_t	V4l2OpenDevices();
	int32_t	V4l2Link();
	int32_t	V4l2SetFormat();
	int32_t	V4l2CreateBuffer();
	int32_t	V4l2CalcAllocSize(uint32_t width, uint32_t height, uint32_t format);
	void	V4l2Deinit();

private:
	enum {	MAX_BUF_NUM = 32 };

	int32_t					m_bInit;

	//	Camera Information
	int32_t					m_iModule;

	int32_t					m_iSensorId;
	int32_t					m_bIsMiPi;
	int32_t					m_bInterlaced;

	//	Source In/Out/Crop Size
	int32_t					m_iWidth, m_iHeight;
	int32_t					m_iCropX, m_iCropY;
	int32_t					m_iCropWidth, m_iCropHeight;

	//	Memory
	int32_t					m_iNumPlane;
	int32_t					m_iPixelFormat;

	//	Bus
	int32_t					m_iBusFormat;
	//	File Descriptor
	int32_t					m_iSensorFd;
	int32_t					m_iClipperSubdevFd;
	int32_t					m_iDecimatorSubdevFd;
	int32_t					m_iCsiSubdevFd;
	int32_t					m_iClipperVideoFd;
	int32_t					m_iDecimatorVideoFd;

	//	External Buffer Information
	NX_VID_MEMORY_INFO		*m_pMemSlot[MAX_BUF_NUM];
	int32_t					m_iCurQueuedSize;
	int32_t					m_iRegBufSize;
	pthread_mutex_t			m_hLock;


#ifndef USE_ION_ALLOCATOR
	int32_t					m_iDrmFd;
	int32_t					m_iDmaFds[MAX_BUF_NUM][NX_MAX_PLANES];
	int32_t					m_iGemFds[MAX_BUF_NUM][NX_MAX_PLANES];
#else
	int32_t					sharedFd[MAX_BUF_NUM][NX_MAX_PLANES];
#endif
	int32_t					m_iBufferSize[MAX_BUF_NUM][NX_MAX_PLANES];
	int32_t					m_iStride[MAX_BUF_NUM][NX_MAX_PLANES];


	void*					m_pVirAddr[MAX_BUF_NUM][NX_MAX_PLANES];





private:
	NX_CV4l2Camera (NX_CV4l2Camera &Ref);
	NX_CV4l2Camera &operator=(NX_CV4l2Camera &Ref);
};

#endif	// __cplusplus
#endif	// __NX_CV4L2CAMERA_H__