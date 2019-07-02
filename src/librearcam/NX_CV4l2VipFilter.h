//------------------------------------------------------------------------------
//
//	Copyright (C) 2015 Nexell Co. All Rights Reserved
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

#ifndef __NX_CV4L2VIPFILTER_H__
#define __NX_CV4L2VIPFILTER_H__

#ifdef __cplusplus

#include <NX_CBaseFilter.h>
#include <NX_CV4l2Camera.h>
#ifdef ANDROID_SURF_RENDERING
#include "NX_CAndroidRenderer.h"
#endif

class NX_CV4l2VipOutputPin;

//------------------------------------------------------------------------------
class NX_CV4l2VipFilter
	: public NX_CBaseFilter
{
public:
	NX_CV4l2VipFilter();
	virtual ~NX_CV4l2VipFilter();

public:
	virtual void*	FindInterface( const char*  pFilterId, const char* pFilterName, const char* pInterfaceId );
	virtual NX_CBasePin* FindPin( int32_t iDirection, int32_t iIndex );

	virtual void	GetFilterInfo( NX_FILTER_INFO *pInfo );

	virtual int32_t	Run( void );
	virtual int32_t Stop( void );
	virtual int32_t Pause( int32_t );

	void GetResolution(int32_t type, int32_t module, int32_t *width, int32_t *height);


#ifdef ANDROID_SURF_RENDERING
	virtual int32_t SetConfig( NX_MEDIA_INFO *pInfo , NX_CAndroidRenderer *pAndroidRender);
#else
	virtual int32_t SetConfig( NX_MEDIA_INFO *pInfo);
#endif

	virtual int32_t	Capture( int32_t iQuality );
	virtual void	RegFileNameCallback( int32_t (*cbFunc)(uint8_t*, uint32_t) );

	int32_t 		ReleaseBuffer( void *pBuf );
	//virtual int32_t         GetVidMemorySharedFD( void );

			void	VipQueueBuffer( int32_t bufIndex );

private:
	static void*	ThreadStub( void *pObj );
	void			ThreadProc( void );

	int32_t	Init( void );
	int32_t	Deinit( void );



private:
	enum { MAX_INPUT_NUM = 256, MAX_OUTPUT_NUM = 8 };
	enum { MAX_FILENAME_SIZE = 1024 };
	enum { MAX_NUM_BUFFER=32, NUM_BUFFER=8 };

	NX_CV4l2VipOutputPin*	m_pOutputPin;

	pthread_t		m_hThread;
	int32_t			m_bThreadRun;
	int32_t			m_bPause;
	int32_t 		getFirstFrame;

	NX_CV4l2Camera*	m_pV4l2Camera;
	NX_CQueue*		m_pReleaseQueue;

	NX_VID_MEMORY_HANDLE *m_hVideoMemory;

	NX_CMutex		m_hLock;

private:
	int32_t			m_bCapture;
	int32_t			m_iCaptureQuality;
	int32_t			m_iBufferIdx;
	char*			m_pFileName;
	int32_t 			preview_idx;

	int32_t			(*FileNameCallbackFunc)( uint8_t *pBuf, uint32_t iBufSize );

	NX_CSemaphore*		m_pSemV4l2Vip;

#ifdef ANDROID_SURF_RENDERING
	NX_CAndroidRenderer *m_pAndroidRender;
#endif


private:
	NX_CV4l2VipFilter (const NX_CV4l2VipFilter &Ref);
	NX_CV4l2VipFilter &operator=(const NX_CV4l2VipFilter &Ref);
};

//------------------------------------------------------------------------------
class NX_CV4l2VipOutputPin
	: public NX_CBaseOutputPin
{
public:
	NX_CV4l2VipOutputPin();
	virtual ~NX_CV4l2VipOutputPin();

public:
	virtual int32_t	ReleaseSample( NX_CSample *pSample );
	virtual int32_t GetDeliverySample( NX_CSample **ppSample );

	int32_t AllocateBuffer( int32_t iNumOfBuffer );
	void	FreeBuffer( void );
	void	ResetSignal( void );

private:
	NX_CSampleQueue*	m_pSampleQueue;
	NX_CSemaphore*		m_pSemQueue;

	int32_t				m_iNumOfBuffer;

private:
	NX_CV4l2VipOutputPin (const NX_CV4l2VipOutputPin &Ref);
	NX_CV4l2VipOutputPin &operator=(NX_CV4l2VipOutputPin &Ref);
};

#endif	// __cplusplus

#endif	// __NX_CV4L2VIPFILTER_H__
