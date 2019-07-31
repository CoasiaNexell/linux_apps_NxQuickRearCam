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

#ifndef __NX_CDRMVIDEORENDERFILTER_H__
#define __NX_CDRMVIDEORENDERFILTER_H__

#ifdef __cplusplus

#include "NX_CBaseFilter.h"
//#include <CNX_BaseClass.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
//#include <nx_video_api.h>
#include <nxp_video_alloc.h>

#ifdef ANDROID_SURF_RENDERING
#include "NX_CAndroidRenderer.h"
#endif


typedef struct DSP_IMG_RECT {
	int32_t		left;
	int32_t		top;
	int32_t		right;
	int32_t		bottom;
} DSP_IMG_RECT;

typedef struct NX_DISPLAY_INFO {
	uint32_t		connectorId;
	uint32_t		planeId;
	uint32_t		ctrlId;

	int32_t			width;		// source width
	int32_t			height;		// source height
	int32_t			stride;		// source image's strid
	int32_t			drmFormat;	// source image's format
	int32_t			numPlane;	// source image's plane number
	int32_t			drmFd;		//	Drm Device Handle

	DSP_IMG_RECT 	dspSrcRect;	// source image's crop region
	DSP_IMG_RECT	dspDstRect;	// target display rect

	int32_t			lcd_width;
	int32_t			lcd_height;
	int32_t			setCrtc;

	int32_t			pglEnable; // pgl drawing enable

	void *pNativeWindow;
} NX_DISPLAY_INFO;

typedef struct NX_DISPLAY_INFO	*NX_DISPLAY_HANDLE;

#define MAX_INPUT_BUFFER		32
#define MAX_ALLOC_BUFFER		8
#define MAX_PLANE_NUM			4

typedef struct
{
	int32_t		width;			//	Video Image's Width
	int32_t		height;			//	Video Image's Height
	int32_t		align;			//	Start address align
	int32_t		planes;			//	Number of valid planes
	int32_t		pixelByte;		//	Pixel Bytes
	uint32_t	format;			//	Pixel Format

	int32_t		drmFd;					//	Drm Device Handle
	int32_t		dmaFd[MAX_PLANE_NUM];	//	DMA memory Handle
	int32_t		gemFd[MAX_PLANE_NUM];	//	GEM Handle
	int32_t		size[MAX_PLANE_NUM];	//	Each plane's size.
	int32_t		stride[MAX_PLANE_NUM];	//	Each plane's stride.
	void*		buffer[MAX_PLANE_NUM];	//	virtual address.
} NX_VID_MEMORY;


class NX_CVideoRenderInputPin;

//------------------------------------------------------------------------------
class NX_CVideoRenderFilter
	: public NX_CBaseFilter
{
public:
	NX_CVideoRenderFilter();
	virtual ~NX_CVideoRenderFilter();

public:
	virtual void*	FindInterface( const char*  pFilterId, const char* pFilterName, const char* pInterfaceId );
	virtual NX_CBasePin* FindPin( int32_t iDirection, int32_t iIndex );

	virtual void	GetFilterInfo( NX_FILTER_INFO *pInfo );

	//virtual int32_t Init( /*NX_DISPLAY_INFO *pDspInfo*/ );

	virtual int32_t Run( void );
	virtual int32_t Stop( void );
	virtual int32_t Pause( int32_t );

public:
	//
	//	Interface Function
	//
	int32_t		EnableRender( int32_t bEnable );

	int32_t		SetCrop( int32_t iX, int32_t iY, int32_t iWidth, int32_t iHeight );
	int32_t		SetPosition( int32_t iX, int32_t iY, int32_t iWidth, int32_t iHeight );
	int32_t		SetVideoLayerPriority( int32_t iPriority );

	int32_t		GetBufferConfig( int32_t *iNumOfBuffer, int32_t *iWidth, int32_t *iHeight );
	int32_t		SetBufferSharedFD( int32_t *pSharedFD );
	int32_t		SetBufferPhyAddr( int32_t *pPhyAddr );

#ifdef ANDROID_SURF_RENDERING
	int32_t		SetConfig( NX_DISPLAY_INFO *pDspInfo, NX_CAndroidRenderer *m_pAndroidRender);
#else
	int32_t		SetConfig( NX_DISPLAY_INFO *pDspInfo);
#endif
private:
	static void *ThreadStub( void *pObj );
	void		ThreadProc( void );

	int32_t 	Init( void );
	int32_t 	Deinit( void );

	int32_t		Render( NX_CSample *pSample );

	int32_t 	DspVideoSetPriority(int32_t priority);

	int32_t 	DrmIoctl( int32_t fd, unsigned long request, void *pArg );
	int32_t		ImportGemFromFlink( int32_t fd, uint32_t flinkName );

private:
	enum { MAX_INPUT_NUM = 256, MAX_OUTPUT_NUM = 12 };
	enum { MAX_VIDEO_MEMORY_NUM = 8 };

	NX_CVideoRenderInputPin		*m_pInputPin;

	pthread_t		m_hThread;
	int32_t			m_bThreadRun;
	int32_t			m_bPause;

	NX_VID_MEMORY_INFO*			m_pVideoMemory[MAX_VIDEO_MEMORY_NUM];
	NX_VID_MEMORY_HANDLE*		m_hVideoMemory;

	NX_CSample*		m_pPrvSample;

	int32_t			m_iDspIndex;
	int32_t			m_SharedFD[MAX_VIDEO_MEMORY_NUM];
	int32_t			m_PhyAddr[MAX_VIDEO_MEMORY_NUM];

	NX_CQueue*		m_pIdxQueue;
	NX_CQueue*		m_pDspQueue;

	int32_t			m_iSurfaceX;
	int32_t			m_iSurfaceY;
	int32_t			m_iSurfaceWidth;
	int32_t			m_iSurfaceHeight;


	NX_CMutex		m_hLock;



	uint32_t		m_BufferId[MAX_INPUT_BUFFER];
	uint32_t		m_plBufferId;

	NX_DISPLAY_INFO	m_DspInfo;

	//	Last Image's Information
	int32_t			m_LastBufferIndex;

	int32_t 		getFirstFrame;


	//pthread_mutex_t	m_hDspCtrlLock;
	NX_CMutex	m_hDspCtrlLock;

	void *pNativeWindow;

#ifdef ANDROID
	struct dp_framebuffer* default_fb;
	struct dp_device *drm_device;
#endif

#ifdef ANDROID_SURF_RENDERING
	NX_CAndroidRenderer *m_pAndroidRender;
#endif

private:
	NX_CVideoRenderFilter (const NX_CVideoRenderFilter &Ref);
	NX_CVideoRenderFilter &operator=(const NX_CVideoRenderFilter &Ref);
};

//------------------------------------------------------------------------------
class NX_CVideoRenderInputPin
	: public NX_CBaseInputPin
{
public:
	NX_CVideoRenderInputPin();
	virtual ~NX_CVideoRenderInputPin();

public:
	virtual int32_t Receive( NX_CSample *pSample );
	virtual int32_t GetSample( NX_CSample **ppSample );
	virtual int32_t Flush( void );

	virtual int32_t PinNegotiation( NX_CBaseOutputPin *pOutPin );

	int32_t	AllocateBuffer( int32_t iNumOfBuffer );
	void	FreeBuffer( void );
	void	ResetSignal( void );

private:
	NX_CSampleQueue		*m_pSampleQueue;
	NX_CSemaphore		*m_pSemQueue;

private:
	NX_CVideoRenderInputPin (const NX_CVideoRenderInputPin &Ref);
	NX_CVideoRenderInputPin &operator=(const NX_CVideoRenderInputPin &Ref);
};



#endif	// __cplusplus

#endif	// __NX_CDRMVIDEORENDERFILTER_H__
