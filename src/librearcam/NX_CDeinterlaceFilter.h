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

#ifndef __NX_CDEINTERLACEFILTER_H__
#define __NX_CDEINTERLACEFILTER_H__

#ifdef __cplusplus
#include <NX_CBaseFilter.h>
//#include <nx_video_api.h>
#include <nxp_video_alloc.h>

#ifdef ANDROID_SURF_RENDERING
#include <NX_CAndroidRenderer.h>
#endif

enum nx_deinter_engine {
	NON_SW_DEINTERLACER = 0,
	NEXELL_SW_DEINTERLACER,
	THUNDER_SW_DEINTERLACER,
	DEINTERLACER_ENGINE_MAX
};

typedef struct NX_DEINTERLACE_INFO {
	int32_t width;
	int32_t height;
	int32_t engineSel;
	int32_t corr;
} NX_DEINTERLACE_INFO;

class NX_CDeiniterlaceInputPin;
class NX_CDeiniterlaceOutputPin;

//------------------------------------------------------------------------------
class NX_CDeinterlaceFilter
	: public NX_CBaseFilter
{
public:
	NX_CDeinterlaceFilter();
	virtual ~NX_CDeinterlaceFilter();

public:
	virtual void*	FindInterface( const char*  pFilterId, const char* pFilterName, const char* pInterfaceId );
	virtual NX_CBasePin* FindPin( int32_t iDirection, int32_t iIndex );

	virtual void	GetFilterInfo( NX_FILTER_INFO *pInfo );

	virtual int32_t Run( void );
	virtual int32_t Stop( void );
	virtual int32_t Pause( int32_t );

	virtual int32_t	Capture( int32_t iQuality );
	virtual void	RegFileNameCallback( int32_t (*cbFunc)(uint8_t*, uint32_t) );


#ifdef ANDROID_SURF_RENDERING
	int32_t SetConfig (NX_DEINTERLACE_INFO *pDeinterInf, NX_CAndroidRenderer *m_pAndroidRender);
	NX_CAndroidRenderer *m_pAndroidRender;
#else
	int32_t SetConfig (NX_DEINTERLACE_INFO *pDeinterInf);
#endif

private:
	static void *ThreadStub( void *pObj );
	void		ThreadProc( void );

	int32_t 	Init( void );
	int32_t 	Deinit( void );

	int32_t 	Deinterlace( NX_CSample *pInSample, NX_CSample *pOutSample );


private:
	enum { MAX_INPUT_NUM = 256, MAX_OUTPUT_NUM = 8 };
	enum { MAX_FILENAME_SIZE = 1024 };

	NX_CDeiniterlaceInputPin *m_pInputPin;
	NX_CDeiniterlaceOutputPin *m_pOutputPin;

	pthread_t		m_hThread;
	int32_t			m_bThreadRun;

	NX_CMutex		m_hLock;

	void *hDeInterlace;

	NX_DEINTERLACE_INFO	m_DeinterInfo;

private:
	int32_t			m_bCapture;
	int32_t			m_iCaptureQuality;
	char*			m_pFileName;
	int32_t			(*FileNameCallbackFunc)( uint8_t *pBuf, uint32_t iBufSize );

private:
	NX_CDeinterlaceFilter (const NX_CDeinterlaceFilter &Ref);
	NX_CDeinterlaceFilter &operator=(const NX_CDeinterlaceFilter &Ref);
};

//------------------------------------------------------------------------------
class NX_CDeiniterlaceInputPin
	: public NX_CBaseInputPin
{
public:
	NX_CDeiniterlaceInputPin();
	virtual ~NX_CDeiniterlaceInputPin();

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
	NX_CDeiniterlaceInputPin (const NX_CDeiniterlaceInputPin &Ref);
	NX_CDeiniterlaceInputPin &operator=(const NX_CDeiniterlaceInputPin &Ref);
};

//------------------------------------------------------------------------------
class NX_CDeiniterlaceOutputPin
	: public NX_CBaseOutputPin
{
public:
	NX_CDeiniterlaceOutputPin();
	virtual ~NX_CDeiniterlaceOutputPin();

public:
	virtual int32_t ReleaseSample( NX_CSample *pSample );
	virtual int32_t GetDeliverySample( NX_CSample **ppSample );

#ifndef ANDROID_SURF_RENDERING
	int32_t	AllocateBuffer( int32_t iNumOfBuffer );
#else
	int32_t	AllocateBuffer( int32_t iNumOfBuffer , NX_CAndroidRenderer *pAndroidRender);
#endif
	void	FreeBuffer( void );
	void 	ResetSignal( void );

private:
	NX_CSampleQueue		*m_pSampleQueue;
	NX_CSemaphore		*m_pSemQueue;

	NX_VID_MEMORY_HANDLE *m_hVideoMemory;
	int32_t				m_iNumOfBuffer;


private:
	NX_CDeiniterlaceOutputPin (const NX_CDeiniterlaceOutputPin &Ref);
	NX_CDeiniterlaceOutputPin &operator=(const NX_CDeiniterlaceOutputPin &Ref);
};

#endif	// __cplusplus

#endif	// __NX_CDEINTERLACEFILTER_H__
