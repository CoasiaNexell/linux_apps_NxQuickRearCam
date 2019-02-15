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

#include <string.h>
#include <fcntl.h>
//#include <nx_fourcc.h>
#include <drm_fourcc.h>
#include <nxp_video_alloc.h>
#include <nx_deinterlace.h>
#include "NX_CDeinterlaceFilter.h"

#ifdef __cplusplus
extern "C"{
#endif

#ifdef THUNDER_DEINTERLACE_TEST
#include <deinterlacer.h>
#endif

#ifdef __cplusplus
}
#endif


#define NX_FILTER_ID		"NX_FILTER_DEINTERLACE"

// #define NX_DBG_OFF

//#define BY_PASS

////////////////////////////////////////////////////////////////////////////////
//
//	NX_CDeinterlaceFilter
//
#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_CDeinterlaceFilter]"
#include <NX_DbgMsg.h>

#define SCALE        (255)
#define CORR         (3)
#define BIAS         (8)

#define THREAD_PRIORITY_CHANGE	0

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
NX_CDeinterlaceFilter::NX_CDeinterlaceFilter()
	: m_pInputPin( NULL )
	, m_pOutputPin( NULL )
	, m_hThread( 0x00 )
	, m_bThreadRun( false )
	, m_bCapture( false )
	, m_iCaptureQuality( 100 )
{
	SetFilterId( NX_FILTER_ID );

	NX_PIN_INFO info;
	m_pInputPin		= new NX_CDeiniterlaceInputPin();
	info.iIndex 	= m_FilterInfo.iInPinNum;
	info.iDirection	= NX_PIN_INPUT;
	m_pInputPin->SetOwner( this );
	m_pInputPin->SetPinInfo( &info );
	m_FilterInfo.iInPinNum++;

	m_pOutputPin	= new NX_CDeiniterlaceOutputPin();
	info.iIndex		= m_FilterInfo.iOutPinNum;
	info.iDirection	= NX_PIN_OUTPUT;
	m_pOutputPin->SetOwner( this );
	m_pOutputPin->SetPinInfo( &info );
	m_FilterInfo.iOutPinNum++;

	hDeInterlace = NULL;

}

//------------------------------------------------------------------------------
NX_CDeinterlaceFilter::~NX_CDeinterlaceFilter()
{
	Deinit();

	if( m_pInputPin )
		delete m_pInputPin;

	if( m_pOutputPin )
		delete m_pOutputPin;
}

//------------------------------------------------------------------------------
void* NX_CDeinterlaceFilter::FindInterface( const char*  pFilterId, const char* pFilterName, const char* /*pInterfaceId*/ )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	if( !strcmp( (char*)GetFilterId(),		pFilterId )		||
		!strcmp( (char*)GetFilterName(),	pFilterName ) )
	{
		NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
		return (NX_CDeinterlaceFilter*)this;
	}


	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return NULL;
}

//------------------------------------------------------------------------------
NX_CBasePin* NX_CDeinterlaceFilter::FindPin( int32_t iDirection, int32_t iIndex )
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
void NX_CDeinterlaceFilter::GetFilterInfo( NX_FILTER_INFO *pInfo )
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
int32_t NX_CDeinterlaceFilter::SetConfig (NX_DEINTERLACE_INFO *pDeinterInfo, NX_CAndroidRenderer* pAndroidRender )
#else
int32_t NX_CDeinterlaceFilter::SetConfig (NX_DEINTERLACE_INFO *pDeinterInfo)
#endif
{
	m_DeinterInfo.width 	= pDeinterInfo->width;
	m_DeinterInfo.height 	= pDeinterInfo->height;
	m_DeinterInfo.engineSel = pDeinterInfo->engineSel;
	m_DeinterInfo.corr		= pDeinterInfo->corr;

#ifdef ANDROID_SURF_RENDERING
	m_pAndroidRender = pAndroidRender;
#endif

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CDeinterlaceFilter::Run( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );
	int ret = 0;

#if THREAD_PRIORITY_CHANGE
	pthread_attr_t thread_attrs;
	struct sched_param param;
	struct sched_param param_res;

#endif

	if( false == m_bRun )
	{
		if( m_pInputPin )
			m_pInputPin->Active();

		if( m_pOutputPin && m_pOutputPin->IsConnected() )
			m_pOutputPin->Active();

		if( 0 > Init() ) {
			NxDbgMsg( NX_DBG_ERR, "Fail, Init().\n" );
			return -1;
		}
#if THREAD_PRIORITY_CHANGE
		ret = pthread_attr_init(&thread_attrs);
		if(ret < 0)
		{
			NxDbgMsg( NX_DBG_ERR, "Fail, pthread_attr_init\n" );
		}

		ret = pthread_attr_setinheritsched(&thread_attrs, PTHREAD_EXPLICIT_SCHED);
		if(ret < 0)
		{
			NxDbgMsg( NX_DBG_ERR, "Fail, pthread_attr_setinheritsched\n" );
		}

		ret = pthread_attr_setschedpolicy(&thread_attrs, SCHED_RR);
		//ret = pthread_attr_setschedpolicy(&thread_attrs, SCHED_OTHER);

		if(ret < 0)
		{
			NxDbgMsg( NX_DBG_ERR, "Fail, pthread_attr_setschedpolicy\n" );
		}

		param.__sched_priority = 50;

		ret = pthread_attr_setschedparam(&thread_attrs, &param);


		if(ret < 0)
		{
			NxDbgMsg( NX_DBG_ERR, "Fail, pthread_attr_setschedparam\n" );
		}

		ret	= pthread_attr_getschedparam(&thread_attrs, &param_res);
		NxDbgMsg( NX_DBG_INFO,"ret : %d ======priority : %d\n",ret, param_res.__sched_priority );
#endif
		m_bThreadRun = true;

#if THREAD_PRIORITY_CHANGE
		if( 0 > pthread_create( &this->m_hThread, &thread_attrs, this->ThreadStub, this ) ) {
			NxDbgMsg( NX_DBG_ERR, "Fail, Create Thread.\n" );
			return -1;
		}

		pthread_attr_destroy(&thread_attrs);
		if(ret < 0)
		{
			NxDbgMsg( NX_DBG_ERR, "Fail, pthread_attr_destroy\n" );
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
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CDeinterlaceFilter::Stop( void )
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

		if(m_DeinterInfo.engineSel == NEXELL_SW_DEINTERLACER)
		{
			if ( hDeInterlace )
			NX_DeInterlaceClose( hDeInterlace );
		}
#ifdef THUNDER_DEINTERLACE_TEST
		else if(m_DeinterInfo.engineSel == THUNDER_SW_DEINTERLACER)
		{
			deinterlacer_destroy();
		}
#endif

		Deinit();

		m_bRun = false;
	}

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CDeinterlaceFilter::Pause( int32_t mbPause )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CDeinterlaceFilter::Capture( int32_t iQuality )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	m_bCapture = true;
	m_iCaptureQuality = iQuality;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}


//------------------------------------------------------------------------------
void NX_CDeinterlaceFilter::RegFileNameCallback( int32_t (*cbFunc)(uint8_t*, uint32_t) )
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
int32_t NX_CDeinterlaceFilter::Init( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	m_pInputPin->AllocateBuffer( MAX_INPUT_NUM );

#ifdef ANDROID_SURF_RENDERING
	m_pOutputPin->AllocateBuffer( MAX_OUTPUT_NUM , m_pAndroidRender  );
#else
	m_pOutputPin->AllocateBuffer( MAX_OUTPUT_NUM);
#endif
	if(m_DeinterInfo.engineSel == NEXELL_SW_DEINTERLACER)
	{
		hDeInterlace = NX_DeInterlaceOpen(DEINTERLACE_BLEND);
	}
#ifdef THUNDER_DEINTERLACE_TEST
	else if(m_DeinterInfo.engineSel == THUNDER_SW_DEINTERLACER)
	{
		//deinterlacer_init(m_DeinterInfo.width, m_DeinterInfo.height, CORR, BIAS);
		deinterlacer_init(m_DeinterInfo.width, m_DeinterInfo.height, m_DeinterInfo.corr, BIAS);

		deinterlacer_create();
	}
#endif
	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CDeinterlaceFilter::Deinit( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	m_pInputPin->Flush();
	m_pInputPin->FreeBuffer();
	m_pOutputPin->FreeBuffer();

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
void NX_CDeinterlaceFilter::ThreadProc( void )
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
			NxDbgMsg( NX_DBG_WARN, "Fail, GetSample().\n" );
			continue;
		}

		if( NULL == pInSample ) {
			NxDbgMsg( NX_DBG_WARN, "Fail, Sample is NULL.\n" );
			continue;
		}

		if( 0 > m_pOutputPin->GetDeliverySample( &pOutSample ) ) {
			NxDbgMsg( NX_DBG_WARN, "Fail, GetDeliverySample().\n" );
			pInSample->Unlock();
			continue;
		}

		if( NULL == pOutSample ) {
			NxDbgMsg( NX_DBG_WARN, "Fail, Sample is NULL.\n" );
			pInSample->Unlock();
			continue;
		}

#if TIME_CHECK
		iStartTime = now_ms();
#endif
		if( 0 > Deinterlace( pInSample, pOutSample ) ) {
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
void *NX_CDeinterlaceFilter::ThreadStub( void *pObj )
{
	if( NULL != pObj ) {
		((NX_CDeinterlaceFilter*)pObj)->ThreadProc();
	}

	return (void*)0xDEADDEAD;
}



#define ROUND_UP_16(num) (((num)+15)&~15)
#define ROUND_UP_32(num) (((num)+31)&~31)

//------------------------------------------------------------------------------
int32_t NX_CDeinterlaceFilter::Deinterlace( NX_CSample *pInSample, NX_CSample *pOutSample )
{
	NX_VID_MEMORY_HANDLE hInBuf = NULL;
	int32_t iInBufSize = 0;
	if( 0 > pInSample->GetBuffer( (void**)&hInBuf, &iInBufSize) ) {
		return -1;
	}

	NX_VID_MEMORY_HANDLE hOutBuf = NULL;
	int32_t iOutBufSize = 0;
	if( 0 > pOutSample->GetBuffer( (void**)&hOutBuf, &iOutBufSize) ) {
		return -1;
	}

	if(m_DeinterInfo.engineSel == NEXELL_SW_DEINTERLACER)
	{
		NX_DEINTERLACER_INFO hInInfo;
		NX_DEINTERLACER_INFO hOutInfo;

		hInInfo.width = hInBuf->width;
		hInInfo.height = hInBuf->height;

		hInInfo.stride[0] = hInBuf->stride[0];
		hInInfo.stride[1] = hInBuf->stride[1];
		hInInfo.stride[2] = hInBuf->stride[2];

		hOutInfo.width = hOutBuf->width;
		hOutInfo.height = hOutBuf->height;

		hOutInfo.stride[0] = hOutBuf->stride[0];
		hOutInfo.stride[1] = hOutBuf->stride[1];
		hOutInfo.stride[2] = hOutBuf->stride[2];

		hInInfo.pY = (uint8_t*)(hInBuf->pBuffer[0]);
		hInInfo.pU = hInInfo.pY + (hInInfo.stride[0] * ROUND_UP_16(hInInfo.height));
		hInInfo.pV = hInInfo.pU + (hInInfo.stride[1] * ROUND_UP_16(hInInfo.height>>1));

#ifdef ANDROID_SURF_RENDERING
		hOutInfo.pY =  (uint8_t*)(hOutBuf->pBuffer[0]);
		hOutInfo.pV = hOutInfo.pY + (hOutInfo.stride[0] * ROUND_UP_16(hOutInfo.height));
		hOutInfo.pU = hOutInfo.pV + (hOutInfo.stride[1] * ROUND_UP_16(hOutInfo.height>>1));
#else
		hOutInfo.pY =  (uint8_t*)(hOutBuf->pBuffer[0]);
		hOutInfo.pU = hOutInfo.pY + (hOutInfo.stride[0] * ROUND_UP_16(hOutInfo.height));
		hOutInfo.pV = hOutInfo.pU + (hOutInfo.stride[1] * ROUND_UP_16(hOutInfo.height>>1));
#endif

		if(NX_DeInterlaceFrame( hDeInterlace, &hInInfo, 0, &hOutInfo ) < 0)
		{
		 	printf("NX_DeInterlaceFrame() failed!!!\n");
		}

	}
#ifdef THUNDER_DEINTERLACE_TEST
	else if(m_DeinterInfo.engineSel == THUNDER_SW_DEINTERLACER)
	{

		uint8_t *in_data[3];
		uint8_t *out_data[3];
		int32_t in_strideWidth[3] = {0,};
		int32_t in_strideHeight[3] = {0,};
		int32_t out_strideWidth[3] = {0,};
		int32_t out_strideHeight[3] = {0,};

		in_strideWidth[0] = hInBuf->stride[0];
		in_strideWidth[1] = hInBuf->stride[1];
		in_strideWidth[2] = hInBuf->stride[2];
		in_strideHeight[0] = ROUND_UP_16(hInBuf->height);
		in_strideHeight[1] = ROUND_UP_16(hInBuf->height >> 1);
		in_strideHeight[2] = in_strideHeight[1];

		out_strideWidth[0] = hOutBuf->stride[0];
		out_strideWidth[1] = hOutBuf->stride[1];
		out_strideWidth[2] = hOutBuf->stride[2];
		out_strideHeight[0] = ROUND_UP_16(hOutBuf->height);
		out_strideHeight[1] = ROUND_UP_16(hOutBuf->height >> 1);
		out_strideHeight[2] = out_strideHeight[1];

#ifdef ANDROID_SURF_RENDERING
		out_data[0] = (uint8_t *)hOutBuf->pBuffer[0];  // y buffer frame out
		out_data[2] = (uint8_t *)(out_data[0] + (out_strideWidth[0]*out_strideHeight[0]));  // u buffer frame out
		out_data[1] = (uint8_t *)(out_data[2] + (out_strideWidth[1]*out_strideHeight[1]));  // v buffer frame out
#else
		out_data[0] = (uint8_t *)hOutBuf->pBuffer[0];  // y buffer frame out
		out_data[1] = (uint8_t *)(out_data[0] + (out_strideWidth[0]*out_strideHeight[0]));  // u buffer frame out
		out_data[2] = (uint8_t *)(out_data[1] + (out_strideWidth[1]*out_strideHeight[1]));  // v buffer frame out
#endif
		in_data[0]  = (uint8_t *)hInBuf->pBuffer[0]; // y buffer luma
		in_data[1]  = (uint8_t *)(in_data[0] + (in_strideWidth[0]*in_strideHeight[0]));   // u buffer luma
		in_data[2]  = (uint8_t *)(in_data[1] + (in_strideWidth[1]*in_strideHeight[1]));  // v buffer luma

		deinterlacer_run(in_data, in_strideWidth, out_data, out_strideWidth);

	}
#endif
	else
	{
		uint8_t *pSrcBuf = NULL;
		uint8_t *pDstBuf = NULL;

		int32_t in_strideWidth[3] = {0,};
		int32_t in_strideHeight[3] = {0,};
		int32_t out_strideWidth[3] = {0,};
		int32_t out_strideHeight[3] = {0,};


		in_strideWidth[0] = hInBuf->stride[0];
		in_strideWidth[1] = hInBuf->stride[1];
		in_strideWidth[2] = hInBuf->stride[2];
		in_strideHeight[0] = ROUND_UP_16(hInBuf->height);
		in_strideHeight[1] = ROUND_UP_16(hInBuf->height >> 1);
		in_strideHeight[2] = in_strideHeight[1];

		out_strideWidth[0] = hOutBuf->stride[0];
		out_strideWidth[1] = hOutBuf->stride[1];
		out_strideWidth[2] = hOutBuf->stride[2];
		out_strideHeight[0] = ROUND_UP_16(hOutBuf->height);
		out_strideHeight[1] = ROUND_UP_16(hOutBuf->height >> 1);
		out_strideHeight[2] = out_strideHeight[1];


		pSrcBuf = (uint8_t*)(hInBuf->pBuffer[0]);
		pDstBuf = (uint8_t*)(hOutBuf->pBuffer[0]);


		for(int32_t i=0; i<out_strideHeight[0]; i++)
		{
			memcpy( pDstBuf, pSrcBuf, out_strideWidth[0]);
			pDstBuf += out_strideWidth[0];
			pSrcBuf += in_strideWidth[0];
		}

		for(int32_t i=0; i<out_strideHeight[1]; i++)
		{
			memcpy( pDstBuf, pSrcBuf, out_strideWidth[1]);
			pDstBuf += out_strideWidth[1] ;
			pSrcBuf += in_strideWidth[1] ;
		}

		for(int32_t i=0; i<out_strideHeight[1]; i++)
		{
			memcpy( pDstBuf, pSrcBuf, out_strideWidth[2]);
			pDstBuf += (out_strideWidth[2]);
			pSrcBuf += (in_strideWidth[2]);
		}

	}
#ifndef ANDROID_SURF_RENDERING
	NX_SyncVideoMemory(hOutBuf->drmFd, hOutBuf->gemFd[0], hOutBuf->size[0]);
#endif

	pOutSample->SetTimeStamp( pInSample->GetTimeStamp() );

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	NX_CDeiniterlaceInputPin
//
#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_CDeiniterlaceInputPin]"
#include <NX_DbgMsg.h>

//------------------------------------------------------------------------------
NX_CDeiniterlaceInputPin::NX_CDeiniterlaceInputPin()
	: m_pSampleQueue( NULL )
	, m_pSemQueue( NULL )
{

}

//------------------------------------------------------------------------------
NX_CDeiniterlaceInputPin::~NX_CDeiniterlaceInputPin()
{
	FreeBuffer();
}

//------------------------------------------------------------------------------
int32_t NX_CDeiniterlaceInputPin::Receive( NX_CSample *pSample )
{
	if( NULL == m_pSampleQueue || false == IsActive() )
		return -1;

	pSample->Lock();
	m_pSampleQueue->PushSample( pSample );
	m_pSemQueue->Post();

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CDeiniterlaceInputPin::GetSample( NX_CSample **ppSample )
{
	*ppSample = NULL;

	if( NULL == m_pSampleQueue )
		return -1;

	if( 0 > m_pSemQueue->Pend() )
		return -1;

	return m_pSampleQueue->PopSample( ppSample );
}

//------------------------------------------------------------------------------
int32_t NX_CDeiniterlaceInputPin::Flush( void )
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
int32_t NX_CDeiniterlaceInputPin::PinNegotiation( NX_CBaseOutputPin *pOutPin )
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
int32_t NX_CDeiniterlaceInputPin::AllocateBuffer( int32_t iNumOfBuffer )
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
void NX_CDeiniterlaceInputPin::FreeBuffer( void )
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
void NX_CDeiniterlaceInputPin::ResetSignal( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	if( m_pSemQueue )
		m_pSemQueue->ResetSignal();

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
}


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CDeiniterlaceOutputPin
//
#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_CDeiniterlaceOutputPin]"
#include <NX_DbgMsg.h>

//------------------------------------------------------------------------------
NX_CDeiniterlaceOutputPin::NX_CDeiniterlaceOutputPin()
	: m_pSampleQueue( NULL )
	, m_pSemQueue( NULL )
	, m_iNumOfBuffer( 0 )
{

}

//------------------------------------------------------------------------------
NX_CDeiniterlaceOutputPin::~NX_CDeiniterlaceOutputPin()
{
	FreeBuffer();
}

//------------------------------------------------------------------------------
int32_t NX_CDeiniterlaceOutputPin::ReleaseSample( NX_CSample *pSample )
{
	if( 0 > m_pSampleQueue->PushSample( pSample ) )
		return -1;

	if( 0 > m_pSemQueue->Post() )
		return -1;

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CDeiniterlaceOutputPin::GetDeliverySample( NX_CSample **ppSample )
{
	*ppSample = NULL;

	if( 0 > m_pSemQueue->Pend() )
		return -1;

	if( 0 > m_pSampleQueue->PopSample( ppSample ) )
		return -1;

	(*ppSample)->Lock();

	return 0;
}


#ifndef ALLINEDN
#define ALLINEDN(X,N)	( (X+N-1) & (~(N-1)) )
#endif
//------------------------------------------------------------------------------
#ifdef ANDROID_SURF_RENDERING
int32_t NX_CDeiniterlaceOutputPin::AllocateBuffer( int32_t iNumOfBuffer,  NX_CAndroidRenderer *pAndroidRender)
#else
int32_t NX_CDeiniterlaceOutputPin::AllocateBuffer( int32_t iNumOfBuffer )
#endif
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	//int32_t allocWidth;

	m_pSampleQueue = new NX_CSampleQueue( iNumOfBuffer );
	m_pSampleQueue->ResetValue();

	m_pSemQueue = new NX_CSemaphore( iNumOfBuffer, iNumOfBuffer );
	m_pSemQueue->ResetValue();

	NX_MEDIA_INFO *pInfo = NULL;
	GetMediaInfo( &pInfo );

	pInfo->iNumPlane = 1;

#ifndef ANDROID_SURF_RENDERING
	m_hVideoMemory = (NX_VID_MEMORY_HANDLE*)malloc( sizeof(NX_VID_MEMORY_HANDLE) * iNumOfBuffer );

	for( int32_t i = 0; i < iNumOfBuffer; i++ )
	{
		m_hVideoMemory[i] = NX_AllocateVideoMemory( pInfo->iWidth, pInfo->iHeight, pInfo->iNumPlane, DRM_FORMAT_YUV420, 512, NEXELL_BO_DMA_CACHEABLE );

		if(pInfo->iNumPlane == 1)
		{
			m_hVideoMemory[i]->stride[1] = ALLINEDN((m_hVideoMemory[i]->stride[0] >> 1), 16);
			m_hVideoMemory[i]->stride[2] = ALLINEDN((m_hVideoMemory[i]->stride[0] >> 1), 16);
		}

		NX_MapVideoMemory(m_hVideoMemory[i]);

		NX_CSample *pSample = new NX_CSample();
		pSample->SetOwner( (NX_CBaseOutputPin*)this );
		pSample->SetBuffer( (void*)m_hVideoMemory[i], sizeof(m_hVideoMemory[i]) );

		m_pSampleQueue->PushSample( (NX_CSample*)pSample );
		pSample->SetIndex(i);
	}
#else
	pAndroidRender->GetBuffers(iNumOfBuffer, pInfo->iWidth, pInfo->iHeight, &m_hVideoMemory);

	for( int32_t i = 0; i < iNumOfBuffer; i++ )
	{
		NX_CSample *pSample = new NX_CSample();

		pSample->SetOwner( (NX_CBaseOutputPin*)this );
		pSample->SetBuffer( (void*)m_hVideoMemory[i], sizeof(m_hVideoMemory[i]) );

		m_pSampleQueue->PushSample( (NX_CSample*)pSample );
		pSample->SetIndex(i);
	}
#endif
	m_iNumOfBuffer = iNumOfBuffer;
	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
void NX_CDeiniterlaceOutputPin::FreeBuffer( void )
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
				NX_VID_MEMORY_HANDLE hVidMem = NULL;
				int32_t iSize = 0;

				pSample->GetBuffer( (void**)&hVidMem, &iSize );
#ifndef ANDROID_SURF_RENDERING
				if( hVidMem != NULL ) NX_FreeVideoMemory( hVidMem );
#endif
				delete pSample;
			}
		}
#ifndef ANDROID_SURF_RENDERING
		if( m_hVideoMemory ) {
			free( m_hVideoMemory );
			m_hVideoMemory = NULL;
		}
#endif
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
void NX_CDeiniterlaceOutputPin::ResetSignal( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	if( m_pSemQueue )
		m_pSemQueue->ResetSignal();

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
}
