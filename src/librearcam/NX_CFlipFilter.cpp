//------------------------------------------------------------------------------
//
//	Copyright (C) 2021 CoAsiaNexell Co. All Rights Reserved
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

#include <string.h>
#include <fcntl.h>
//#include <nx_fourcc.h>
#include <drm_fourcc.h>
#include <nxp_video_alloc.h>
#include <nx_deinterlace.h>
#include "NX_CFlipFilter.h"
#include "NX_ThreadUtils.h"

#define NX_FILTER_ID		"NX_FILTER_FLIP"

// #define NX_DBG_OFF

//#define BY_PASS

////////////////////////////////////////////////////////////////////////////////
//
//	NX_CFlipFilter
//
#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_CFlipFilter]"
#include <NX_DbgMsg.h>

#define SCALE        (255)
#define CORR         (3)
#define BIAS         (8)

#define TIME_CHECK 	1

#if TIME_CHECK
static double now_ms(void)
{
	struct timespec res;
	clock_gettime(CLOCK_REALTIME, &res);
	return 1000.0*res.tv_sec + (double)res.tv_nsec/1e6;
}
#endif

//------------------------------------------------------------------------------
NX_CFlipFilter::NX_CFlipFilter()
	: m_pInputPin( NULL )
	, m_pOutputPin( NULL )
	, m_hThread( 0x00 )
	, m_bThreadRun( false )
	, m_bCapture( false )
	, m_iCaptureQuality( 100 )
	, m_MemDevFd(-1)
{
	SetFilterId( NX_FILTER_ID );

	NX_PIN_INFO info;
	m_pInputPin		= new NX_CFlipInputPin();
	info.iIndex 	= m_FilterInfo.iInPinNum;
	info.iDirection	= NX_PIN_INPUT;
	m_pInputPin->SetOwner( this );
	m_pInputPin->SetPinInfo( &info );
	m_FilterInfo.iInPinNum++;

	m_pOutputPin	= new NX_CFlipOutputPin();
	info.iIndex		= m_FilterInfo.iOutPinNum;
	info.iDirection	= NX_PIN_OUTPUT;
	m_pOutputPin->SetOwner( this );
	m_pOutputPin->SetPinInfo( &info );
	m_FilterInfo.iOutPinNum++;

}

//------------------------------------------------------------------------------
NX_CFlipFilter::~NX_CFlipFilter()
{
	if( m_pInputPin )
		delete m_pInputPin;

	if( m_pOutputPin )
		delete m_pOutputPin;
}

//------------------------------------------------------------------------------
void* NX_CFlipFilter::FindInterface( const char*  pFilterId, const char* pFilterName, const char* /*pInterfaceId*/ )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	if( !strcmp( (char*)GetFilterId(),		pFilterId )		||
		!strcmp( (char*)GetFilterName(),	pFilterName ) )
	{
		NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
		return (NX_CFlipFilter*)this;
	}


	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return NULL;
}

//------------------------------------------------------------------------------
NX_CBasePin* NX_CFlipFilter::FindPin( int32_t iDirection, int32_t iIndex )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	NX_CBasePin *pBasePin = NULL;
	if( NX_PIN_INPUT == iDirection && 0 == iIndex )
		pBasePin = (NX_CBasePin*)m_pInputPin;
	if( NX_PIN_OUTPUT == iDirection && 0 == iIndex )
		pBasePin = (NX_CBasePin*)m_pOutputPin;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return pBasePin;
}

//------------------------------------------------------------------------------
void NX_CFlipFilter::GetFilterInfo( NX_FILTER_INFO *pInfo )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	pInfo->pFilterId	= (char*)GetFilterId();
	pInfo->pFilterName	= (char*)GetFilterName();
	pInfo->iInPinNum	= m_FilterInfo.iInPinNum;
	pInfo->iOutPinNum	= m_FilterInfo.iOutPinNum;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
}

//------------------------------------------------------------------------------
#ifdef ANDROID_SURF_RENDERING
int32_t NX_CFlipFilter::SetConfig (NX_FLIP_INFO *pFlipInfo, NX_CAndroidRenderer* pAndroidRender )
#else
int32_t NX_CFlipFilter::SetConfig (NX_FLIP_INFO *pFlipInfo)
#endif
{
	m_FlipInfo  = *pFlipInfo;

#ifdef ANDROID_SURF_RENDERING
	m_pAndroidRender = pAndroidRender;
#endif

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CFlipFilter::Run( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );
	int ret = 0;

	if( false == m_bRun )
	{
		m_pInputPin->AllocateBuffer( MAX_INPUT_NUM );
		m_pOutputPin->AllocateBuffer( MAX_OUTPUT_NUM);

		if( m_pInputPin )
			m_pInputPin->Active();

		if( m_pOutputPin && m_pOutputPin->IsConnected() )
			m_pOutputPin->Active();


		//---------------------------------------------------------------

		m_bThreadRun = true;

#ifdef ADJUST_THREAD_PRIORITY
		NX_AdjustThreadPriority(&thread_attrs, SCHED_RR, THREAD_PRIORITY);

		if( 0 > pthread_create( &this->m_hThread, &thread_attrs, this->ThreadStub, this ) ) {
			NxDbgMsg( NX_DBG_ERR, "Fail, Create Thread.\n" );
			return -1;
		}
#else
		if( 0 > pthread_create( &this->m_hThread, NULL, this->ThreadStub, this ) ) {
			NxDbgMsg( NX_DBG_ERR, "Fail, Create Thread.\n" );
			return -1;
		}
#endif
		m_bRun = true;
	}

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return ret;
}

//------------------------------------------------------------------------------
int32_t NX_CFlipFilter::Stop( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	if( true == m_bRun )
	{
		if( m_pInputPin && m_pInputPin->IsActive() )
			m_pInputPin->Inactive();

		if( m_pOutputPin && m_pOutputPin->IsActive() )
			m_pOutputPin->Inactive();

		m_bThreadRun = false;
		m_pInputPin->ResetSignal();
		m_pOutputPin->ResetSignal();

		pthread_join( m_hThread, NULL );


#ifdef ADJUST_THREAD_PRIORITY
		NX_ThreadAttributeDestroy(&thread_attrs);
#endif
		NxDbgMsg( NX_DBG_VBS, "%s()--%d\n", __FUNCTION__, __LINE__ );
		m_pInputPin->Flush();
		m_pInputPin->FreeBuffer();
		m_pOutputPin->FreeBuffer();

		m_bRun = false;

	}

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CFlipFilter::Pause( int32_t mbPause )
{
	(void)(mbPause);
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CFlipFilter::Capture( int32_t iQuality )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	m_bCapture = true;
	m_iCaptureQuality = iQuality;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}


//------------------------------------------------------------------------------
void NX_CFlipFilter::RegFileNameCallback( int32_t (*cbFunc)(uint8_t*, uint32_t) )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	if( cbFunc )
	{
		if( m_pFileName )
		{
			free( m_pFileName );
			m_pFileName = NULL;
		}
		FileNameCallbackFunc = cbFunc;
	}

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
}

//------------------------------------------------------------------------------
void NX_CFlipFilter::SetDeviceFD(int32_t mem_dev_fd)
{
	m_MemDevFd = mem_dev_fd;
}

//------------------------------------------------------------------------------
int32_t NX_CFlipFilter::Init( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	m_pOutputPin->SetDeviceFD(m_MemDevFd);

#ifdef ANDRPOD_SURF_RENDERING
	m_pOutputPin->AllocateVideoBuffer(MAX_OUTPUT_NUM, m_pAndroidRender );
#else
	m_pOutputPin->AllocateVideoBuffer(MAX_OUTPUT_NUM);
#endif

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CFlipFilter::Deinit( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	m_pOutputPin->FreeVideoBuffer();

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
void NX_CFlipFilter::ThreadProc( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	NX_CSample *pInSample = NULL;
	NX_CSample *pOutSample = NULL;
#if TIME_CHECK
	double iStartTime = 0;
	double iEndTime = 0;
	double iCurProcTime = 0;
	double iSumTime = 0;
	double iMinTime    = 0;
	double iMaxTime    = 0;
	int frame_cnt = 0;
	int iGatheringCnt = 100;
#endif
	while( m_bThreadRun )
	{
		if( 0 > m_pInputPin->GetSample( &pInSample) ) {
			//NxDbgMsg( NX_DBG_WARN, "Fail, GetSample().\n" );
			continue;
		}

		if( NULL == pInSample ) {
			//NxDbgMsg( NX_DBG_WARN, "Fail, Sample is NULL.\n" );
			continue;
		}

		if( 0 > m_pOutputPin->GetDeliverySample( &pOutSample ) ) {
			//NxDbgMsg( NX_DBG_WARN, "Fail, GetDeliverySample().\n" );
			pInSample->Unlock();
			continue;
		}

		if( NULL == pOutSample ) {
			//NxDbgMsg( NX_DBG_WARN, "Fail, Sample is NULL.\n" );
			pInSample->Unlock();
			continue;
		}

#if TIME_CHECK
		iStartTime = now_ms();
#endif
		if( 0 > FlipImage( pInSample, pOutSample ) ) {
			NxDbgMsg( NX_DBG_ERR, "Fail, Deinterlace().\n" );
			pInSample->Unlock();
			pOutSample->Unlock();
			continue;
		}

#if TIME_CHECK
		frame_cnt++ ;
		iEndTime = now_ms();
		iCurProcTime = iEndTime-iStartTime;
		iSumTime += iCurProcTime;

		if(frame_cnt == 10)
		{
			iMinTime = iCurProcTime;
			iMaxTime = iCurProcTime;
		}

		if(iMinTime > iCurProcTime)
		{
			iMinTime = iCurProcTime;
		}

		if(iMaxTime < iCurProcTime)
		{
			iMaxTime = iCurProcTime;
		}

		if((frame_cnt%iGatheringCnt) == 0)
		{
			NxDbgMsg(NX_DBG_INFO, "Deinterlace processing time : [Min : %lf] [Max : %lf] [Avg : %lf]\n", iMinTime,  iMaxTime, iSumTime/iGatheringCnt);
			iSumTime = 0;
			iMinTime = iCurProcTime;
			iMaxTime = iCurProcTime;
		}
#endif

		if( 0 > m_pOutputPin->Deliver( pOutSample ) ) {
			// NxDbgMsg( NX_DBG_WARN, "Fail, Deliver().\n" );
		}

		pInSample->Unlock();
		pOutSample->Unlock();

	}

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
}

//------------------------------------------------------------------------------
void *NX_CFlipFilter::ThreadStub( void *pObj )
{
	if( NULL != pObj ) {
		((NX_CFlipFilter*)pObj)->ThreadProc();
	}

	return (void*)0xDEADDEAD;
}



#define ROUND_UP_16(num) (((num)+15)&~15)
#define ROUND_UP_32(num) (((num)+31)&~31)
//------------------------------------------------------------------------------
//	Vertical Flip
void NX_CFlipFilter::FlipV( NX_VID_MEMORY_HANDLE hIn, NX_VID_MEMORY_HANDLE hOut )
{
	uint8_t *pSrcY, *pSrcU, *pSrcV, *pDstY, *pDstU, *pDstV;
	int32_t width = m_FlipInfo.width;
	int32_t height = m_FlipInfo.height;

	pSrcY = (uint8_t*)(hIn->pBuffer[0]);
	pSrcU = pSrcY + (hIn->stride[0] * ROUND_UP_16(hIn->height));
	pSrcV = pSrcU + (hIn->stride[1] * ROUND_UP_16(hIn->height>>1));

	pDstY = (uint8_t*)(hOut->pBuffer[0]);
	pDstU = pDstY + (hOut->stride[0] * ROUND_UP_16(hOut->height));
	pDstV = pDstU + (hOut->stride[1] * ROUND_UP_16(hOut->height>>1));


	//	move to end
	pDstY += height * hOut->stride[0];

	for( int i=0 ; i<height ; i++ )
	{
		pDstY -= hOut->stride[0];
		memcpy( pDstY, pSrcY, width);
		pSrcY += hIn->stride[0];
	}

	width /= 2;
	height /= 2;

	//	U
	pDstU += height * hOut->stride[1];
	for( int i=0 ; i<height ; i++ )
	{
		pDstU -= hOut->stride[1];
		memcpy( pDstU, pSrcU, width);
		pSrcU += hIn->stride[1];
	}

	//	V
	pDstV += height * hOut->stride[2];
	for( int i=0 ; i<height ; i++ )
	{
		pDstV -= hOut->stride[2];
		memcpy( pDstV, pSrcV, width);
		pSrcV += hIn->stride[2];
	}
}

//------------------------------------------------------------------------------
//	Horizontal Flip
void NX_CFlipFilter::FlipH( NX_VID_MEMORY_HANDLE hIn, NX_VID_MEMORY_HANDLE hOut )
{
	uint8_t *pSrcY, *pSrcU, *pSrcV, *pDstY, *pDstU, *pDstV;
	int32_t width = m_FlipInfo.width;
	int32_t height = m_FlipInfo.height;

	pSrcY = (uint8_t*)(hIn->pBuffer[0]);
	pSrcU = pSrcY + (hIn->stride[0] * ROUND_UP_16(hIn->height));
	pSrcV = pSrcU + (hIn->stride[1] * ROUND_UP_16(hIn->height>>1));

	pDstY = (uint8_t*)(hOut->pBuffer[0]);
	pDstU = pDstY + (hOut->stride[0] * ROUND_UP_16(hOut->height));
	pDstV = pDstU + (hOut->stride[1] * ROUND_UP_16(hOut->height>>1));

	for( int i=0 ; i<height ; i++ )
	{
		for( int j=0 ; j<width ; j++ )
		{
			pDstY[i*hOut->stride[0] + j] = pSrcY[i*hIn->stride[0] + (width - 1 - j)];
		}
	}

	width /= 2;
	height /= 2;

	//	U
	for( int i=0 ; i<height ; i++ )
	{
		for( int j=0 ; j<width ; j++ )
		{
			pDstU[i*hOut->stride[1] + j] = pSrcU[i*hIn->stride[1] + (width - 1 - j)];
		}
	}


	//	V
	for( int i=0 ; i<height ; i++ )
	{
		for( int j=0 ; j<width ; j++ )
		{
			pDstV[i*hOut->stride[2] + j] = pSrcV[i*hIn->stride[2] + (width - 1 - j)];
		}
	}

}


//------------------------------------------------------------------------------
//	support just YUV420 format
int32_t NX_CFlipFilter::FlipImage( NX_CSample *pInSample, NX_CSample *pOutSample )
{
	NX_VID_MEMORY_HANDLE hInBuf = NULL;
	NX_VID_MEMORY_HANDLE hOutBuf = NULL;
	int32_t iInBufSize = 0;
	int32_t iOutBufSize = 0;

	if( 0 > pInSample->GetBuffer( (void**)&hInBuf, &iInBufSize) ) {
		return -1;
	}

	if( 0 > pOutSample->GetBuffer( (void**)&hOutBuf, &iOutBufSize) ) {
		return -1;
	}

	if( !hInBuf || !hOutBuf )
	{
		return -1;
	}

	if( m_FlipInfo.direction == FLIP_DIR_HOR )
	{
		FlipH( hInBuf, hOutBuf );
	}

	if( m_FlipInfo.direction == FLIP_DIR_VER )
	{
		FlipV( hInBuf, hOutBuf );
	}

#ifndef ANDROID_SURF_RENDERING
	NX_SyncVideoMemory(hOutBuf->drmFd, hOutBuf->gemFd[0], hOutBuf->size[0]);
#endif

	pOutSample->SetTimeStamp( pInSample->GetTimeStamp() );

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	NX_CFlipInputPin
//
#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_CFlipInputPin]"
#include <NX_DbgMsg.h>

//------------------------------------------------------------------------------
NX_CFlipInputPin::NX_CFlipInputPin()
	: m_pSampleQueue( NULL )
	, m_pSemQueue( NULL )
{

}

//------------------------------------------------------------------------------
NX_CFlipInputPin::~NX_CFlipInputPin()
{
	FreeBuffer();
}

//------------------------------------------------------------------------------
int32_t NX_CFlipInputPin::Receive( NX_CSample *pSample )
{
	if( NULL == m_pSampleQueue || false == IsActive() )
		return -1;

	pSample->Lock();
	m_pSampleQueue->PushSample( pSample );
	m_pSemQueue->Post();

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CFlipInputPin::GetSample( NX_CSample **ppSample )
{
	*ppSample = NULL;

	if( NULL == m_pSampleQueue )
		return -1;

	if( 0 > m_pSemQueue->Pend() )
		return -1;

	return m_pSampleQueue->PopSample( ppSample );
}

//------------------------------------------------------------------------------
int32_t NX_CFlipInputPin::Flush( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	if( NULL == m_pSampleQueue )
	{
		NxDbgMsg( NX_DBG_DEBUG, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	while( 0 < m_pSampleQueue->GetSampleCount() )
	{
		NX_CSample *pSample = NULL;
		if( !m_pSampleQueue->PopSample( &pSample ) ) {
			if( pSample ) {
				pSample->Unlock();
			}
		}
	}

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CFlipInputPin::PinNegotiation( NX_CBaseOutputPin *pOutPin )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	if( NULL == pOutPin ) {
		NxDbgMsg( NX_DBG_WARN, "Fail, OutputPin is NULL.\n" );
		NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	if( pOutPin->IsConnected() )
	{
		NxDbgMsg( NX_DBG_WARN, "Fail, OutputPin is Already Connected.\n" );
		NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	NX_MEDIA_INFO *pInfo = NULL;
	pOutPin->GetMediaInfo( &pInfo );

	if( NX_TYPE_VIDEO != pInfo->iMediaType )
	{
		NxDbgMsg( NX_DBG_WARN, "Fail, MediaType missmatch. ( Require MediaType : NX_TYPE_VIDEO )\n" );
		NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	if( (pInfo->iWidth == 0) || (pInfo->iHeight == 0) )
	{
		NxDbgMsg( NX_DBG_WARN, "Fail, Video Width(%d) / Video Height(%d)\n", pInfo->iWidth, pInfo->iHeight);
		NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	if( NX_FORMAT_YUV != pInfo->iFormat )
	{
		NxDbgMsg( NX_DBG_WARN, "Fail, Format missmatch. ( Require Format : NX_FORMAT_YUV )\n" );
		NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	m_pOwnerFilter->SetMediaInfo( pInfo );
	pOutPin->Connect( this );

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CFlipInputPin::AllocateBuffer( int32_t iNumOfBuffer )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	m_pSampleQueue = new NX_CSampleQueue( iNumOfBuffer );
	m_pSampleQueue->ResetValue();

	m_pSemQueue = new NX_CSemaphore( iNumOfBuffer, 0 );
	m_pSemQueue->ResetValue();

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
void NX_CFlipInputPin::FreeBuffer( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	if( m_pSampleQueue ) {
		delete m_pSampleQueue;
		m_pSampleQueue = NULL;
	}

	if( m_pSemQueue ) {
		delete m_pSemQueue;
		m_pSemQueue = NULL;
	}
	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
}

//------------------------------------------------------------------------------
void NX_CFlipInputPin::ResetSignal( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	if( m_pSemQueue )
		m_pSemQueue->ResetSignal();

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
}


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CFlipOutputPin
//
#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_CFlipOutputPin]"
#include <NX_DbgMsg.h>

//------------------------------------------------------------------------------
NX_CFlipOutputPin::NX_CFlipOutputPin()
	: m_pSampleQueue( NULL )
	, m_pSemQueue( NULL )
	, m_iNumOfBuffer( 0 )
	, m_MemDevFd( -1 )
{

}

//------------------------------------------------------------------------------
NX_CFlipOutputPin::~NX_CFlipOutputPin()
{
	FreeBuffer();
}

//------------------------------------------------------------------------------
int32_t NX_CFlipOutputPin::ReleaseSample( NX_CSample *pSample )
{
	if( 0 > m_pSampleQueue->PushSample( pSample ) )
		return -1;

	if( 0 > m_pSemQueue->Post() )
		return -1;

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CFlipOutputPin::GetDeliverySample( NX_CSample **ppSample )
{
	*ppSample = NULL;

	if( 0 > m_pSemQueue->Pend() )
		return -1;

	if( 0 > m_pSampleQueue->PopSample( ppSample ) )
		return -1;

	(*ppSample)->Lock();

	return 0;
}

//------------------------------------------------------------------------------
void NX_CFlipOutputPin::SetDeviceFD( int32_t  mem_dev_fd)
{
	m_MemDevFd = mem_dev_fd;
}


#ifndef ALLINEDN
#define ALLINEDN(X,N)	( (X+N-1) & (~(N-1)) )
#endif
//------------------------------------------------------------------------------
int32_t NX_CFlipOutputPin::AllocateBuffer( int32_t iNumOfBuffer )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	m_pSampleQueue = new NX_CSampleQueue( iNumOfBuffer );
	m_pSampleQueue->ResetValue();

	m_pSemQueue = new NX_CSemaphore( iNumOfBuffer, iNumOfBuffer );
	m_pSemQueue->ResetValue();


#ifndef ANDROID_SURF_RENDERING
	for( int32_t i = 0; i < iNumOfBuffer; i++ )
	{
		NX_CSample *pSample = new NX_CSample();
		pSample->SetOwner( (NX_CBaseOutputPin*)this );
		pSample->SetBuffer( (void*)m_hVideoMemory[i], sizeof(m_hVideoMemory[i]) );

		m_pSampleQueue->PushSample( (NX_CSample*)pSample );
		pSample->SetIndex(i);
	}
#else

	for( int32_t i = 0; i < iNumOfBuffer; i++ )
	{
		NX_CSample *pSample = new NX_CSample();

		pSample->SetOwner( (NX_CBaseOutputPin*)this );
		pSample->SetBuffer( (void*)m_hVideoMemory[i], sizeof(m_hVideoMemory[i]) );

		m_pSampleQueue->PushSample( (NX_CSample*)pSample );
		pSample->SetIndex(i);
	}
#endif
	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
#ifdef ANDROID_SURF_RENDERING
int32_t NX_CFlipOutputPin::AllocateVideoBuffer( int32_t iNumOfBuffer,  NX_CAndroidRenderer *pAndroidRender)
#else
int32_t NX_CFlipOutputPin::AllocateVideoBuffer( int32_t iNumOfBuffer )
#endif
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_MEDIA_INFO *pInfo = NULL;
	GetMediaInfo( &pInfo );

	printf("Allocate Video Buffer \n");
	pInfo->iNumPlane = 1;

#ifndef ANDROID_SURF_RENDERING
	m_hVideoMemory = (NX_VID_MEMORY_HANDLE*)malloc( sizeof(NX_VID_MEMORY_HANDLE) * iNumOfBuffer );

	for( int32_t i = 0; i < iNumOfBuffer; i++ )
	{
		m_hVideoMemory[i] = NX_AllocateVideoMemory(m_MemDevFd, pInfo->iWidth, pInfo->iHeight, pInfo->iNumPlane, DRM_FORMAT_YUV420, 512, NEXELL_BO_DMA_CACHEABLE );

		if(pInfo->iNumPlane == 1)
		{
			m_hVideoMemory[i]->stride[1] = ALLINEDN((m_hVideoMemory[i]->stride[0] >> 1), 16);
			m_hVideoMemory[i]->stride[2] = ALLINEDN((m_hVideoMemory[i]->stride[0] >> 1), 16);
		}

		NX_MapVideoMemory(m_hVideoMemory[i]);
	}
#else
	pAndroidRender->GetBuffers(iNumOfBuffer, pInfo->iWidth, pInfo->iHeight, &m_hVideoMemory);
#endif
	m_iNumOfBuffer = iNumOfBuffer;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
void NX_CFlipOutputPin::FreeVideoBuffer()
{
#ifndef ANDROID_SURF_RENDERING
	for( int32_t i = 0; i < m_iNumOfBuffer; i++ )
	{
		if(m_hVideoMemory[i])
		{
			NX_FreeVideoMemory(m_hVideoMemory[i]);
			m_hVideoMemory[i] = NULL;
		}
	}
	if(m_hVideoMemory)
	{
		free( m_hVideoMemory );
		m_hVideoMemory = NULL;
	}
#endif
}


//------------------------------------------------------------------------------
void NX_CFlipOutputPin::FreeBuffer( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	if( m_pSampleQueue )
	{
		for( int32_t i = 0; i < m_iNumOfBuffer; i++ )
		{
			NX_CSample *pSample = NULL;
			m_pSampleQueue->PopSample( &pSample );

			if( pSample )
			{
				int32_t iSize = 0;

				delete pSample;
			}
		}
		delete m_pSampleQueue;
		m_pSampleQueue = NULL;
	}

	if( m_pSemQueue )
	{
		delete m_pSemQueue;
		m_pSemQueue = NULL;
	}


	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
}



//------------------------------------------------------------------------------
void NX_CFlipOutputPin::ResetSignal( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	if( m_pSemQueue )
		m_pSemQueue->ResetSignal();

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
}
