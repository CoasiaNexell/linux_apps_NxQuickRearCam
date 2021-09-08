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

#ifndef __NX_CFlipFilter_H__
#define __NX_CFlipFilter_H__

#ifdef __cplusplus
#include <NX_CBaseFilter.h>
//#include <nx_video_api.h>
#include <nxp_video_alloc.h>

#ifdef ANDROID_SURF_RENDERING
#include <NX_CAndroidRenderer.h>
#endif

class NX_CFlipInputPin;
class NX_CFlipOutputPin;

//
//
#define FLIP_DIR_HOR		1
#define FLIP_DIR_VER		2


typedef struct NX_FLIP_INFO {
	int32_t width;
	int32_t height;
	int32_t direction;
} NX_FLIP_INFO;


//------------------------------------------------------------------------------
class NX_CFlipFilter
	: public NX_CBaseFilter
{
public:
	NX_CFlipFilter();
	virtual ~NX_CFlipFilter();

public:
	virtual void*	FindInterface( const char*  pFilterId, const char* pFilterName, const char* pInterfaceId );
	virtual NX_CBasePin* FindPin( int32_t iDirection, int32_t iIndex );

	virtual void	GetFilterInfo( NX_FILTER_INFO *pInfo );

	virtual int32_t Run( void );
	virtual int32_t Stop( void );
	virtual int32_t Pause( int32_t );

	virtual int32_t	Capture( int32_t iQuality );
	virtual void	RegFileNameCallback( int32_t (*cbFunc)(uint8_t*, uint32_t) );

	int32_t 	Init( void );
	int32_t 	Deinit( void );


#ifdef ANDROID_SURF_RENDERING
	int32_t SetConfig (NX_FLIP_INFO *pFlipInfo, NX_CAndroidRenderer *m_pAndroidRender);
	NX_CAndroidRenderer *m_pAndroidRender;
#else
	int32_t SetConfig (NX_FLIP_INFO *pFlipInfo);
#endif
	void		SetDeviceFD(int32_t mem_dev_fd);

private:
	static void *ThreadStub( void *pObj );
	void		ThreadProc( void );

	void 		FlipV( NX_VID_MEMORY_HANDLE hIn, NX_VID_MEMORY_HANDLE hOut );
	void 		FlipH( NX_VID_MEMORY_HANDLE hIn, NX_VID_MEMORY_HANDLE hOut );
	int32_t 	FlipImage( NX_CSample *pInSample, NX_CSample *pOutSample );


private:
	//enum { MAX_INPUT_NUM = 256, MAX_OUTPUT_NUM = 8 };
	enum { MAX_INPUT_NUM = 256, MAX_OUTPUT_NUM = 4 };
	enum { MAX_FILENAME_SIZE = 1024 };

	NX_CFlipInputPin *m_pInputPin;
	NX_CFlipOutputPin *m_pOutputPin;

	pthread_t		m_hThread;
	pthread_attr_t  thread_attrs;
	int32_t			m_bThreadRun;

	NX_CMutex		m_hLock;
	NX_FLIP_INFO	m_FlipInfo;

	void *hDeInterlace;

private:
	int32_t			m_bCapture;
	int32_t			m_iCaptureQuality;
	char*			m_pFileName;
	int32_t			(*FileNameCallbackFunc)( uint8_t *pBuf, uint32_t iBufSize );

	int32_t 		m_MemDevFd;


private:
	NX_CFlipFilter (const NX_CFlipFilter &Ref);
	NX_CFlipFilter &operator=(const NX_CFlipFilter &Ref);
};

//------------------------------------------------------------------------------
class NX_CFlipInputPin
	: public NX_CBaseInputPin
{
public:
	NX_CFlipInputPin();
	virtual ~NX_CFlipInputPin();

public:
	virtual int32_t Receive( NX_CSample *pSample );
	virtual int32_t GetSample( NX_CSample **ppSample );
	virtual int32_t Flush( void );

	virtual int32_t	PinNegotiation( NX_CBaseOutputPin *pOutPin );

	int32_t	AllocateBuffer( int32_t iNumOfBuffer );
	void	FreeBuffer( void );
	void	ResetSignal( void );

private:
	NX_CSampleQueue		*m_pSampleQueue;
	NX_CSemaphore		*m_pSemQueue;

private:
	NX_CFlipInputPin (const NX_CFlipInputPin &Ref);
	NX_CFlipInputPin &operator=(const NX_CFlipInputPin &Ref);
};

//------------------------------------------------------------------------------
class NX_CFlipOutputPin
	: public NX_CBaseOutputPin
{
public:
	NX_CFlipOutputPin();
	virtual ~NX_CFlipOutputPin();

public:
	virtual int32_t ReleaseSample( NX_CSample *pSample );
	virtual int32_t GetDeliverySample( NX_CSample **ppSample );

	void SetDeviceFD(int32_t mem_dev_fd);

#ifndef ANDROID_SURF_RENDERING
	int32_t AllocateVideoBuffer(int32_t iNumOfBuffer);
#else
	int32_t AllocateVideoBuffer( int32_t iNumOfBuffer,  NX_CAndroidRenderer *pAndroidRender);
#endif

	int32_t	AllocateBuffer( int32_t iNumOfBuffer );
	void FreeVideoBuffer();

	void	FreeBuffer( void );
	void 	ResetSignal( void );

private:
	NX_CSampleQueue		*m_pSampleQueue;
	NX_CSemaphore		*m_pSemQueue;

	NX_VID_MEMORY_HANDLE *m_hVideoMemory;
	int32_t				m_iNumOfBuffer;

	int32_t				m_MemDevFd;


private:
	NX_CFlipOutputPin (const NX_CFlipOutputPin &Ref);
	NX_CFlipOutputPin &operator=(const NX_CFlipOutputPin &Ref);
};

#endif	// __cplusplus

#endif	// __NX_CFlipFilter_H__
