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

#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <drm_fourcc.h>
//#include <nx_video_alloc.h>
#include <nxp_video_alloc.h>
#include "NX_CV4l2VipFilter.h"
#include <linux/videodev2.h>
#include "NX_ThreadUtils.h"

#define NX_FILTER_ID		"NX_FILTER_V4L2_VIP"

// #define NX_DBG_OFF

#define DISPLAY_FPS		0

////////////////////////////////////////////////////////////////////////////////
//
//	NX_CV4l2VipFilter
//
#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_CV4l2VipFilter]"
#include <NX_DbgMsg.h>

#if DISPLAY_FPS
static double now_ms(void)
{
	struct timespec res;
	clock_gettime(CLOCK_REALTIME, &res);
	return 1000.0*res.tv_sec + (double)res.tv_nsec/1e6;
}
#endif

//int32_t NX_V4l2DumpMemory( NX_VID_MEMORY_INFO *pInMemory, const char *pOutFile );

//------------------------------------------------------------------------------
NX_CV4l2VipFilter::NX_CV4l2VipFilter()
	: m_pOutputPin( NULL )
	, m_hThread( 0x00 )
	, m_bThreadRun( false )
	, m_bPause(true)
	, m_pV4l2Camera( NULL )
	, m_pReleaseQueue( NULL )
	, m_bCapture( false )
	, m_iCaptureQuality( 100 )
	, m_iBufferIdx(-1)
	, m_pFileName( NULL )
	, FileNameCallbackFunc( NULL )
	, m_CamDevFd(-1)
	, m_MemDevFd(-1)
{
	SetFilterId( NX_FILTER_ID );

	NX_PIN_INFO	info;
	m_pOutputPin	= new NX_CV4l2VipOutputPin();
	info.iIndex		= 0;
	info.iDirection	= NX_PIN_OUTPUT;
	m_pOutputPin->SetOwner( this );
	m_pOutputPin->SetPinInfo( &info );
	m_FilterInfo.iOutPinNum++;

}

//------------------------------------------------------------------------------
NX_CV4l2VipFilter::~NX_CV4l2VipFilter()
{
	if( m_pFileName )
		free( m_pFileName );

	if( m_pOutputPin )
		delete m_pOutputPin;

}

//------------------------------------------------------------------------------
void* NX_CV4l2VipFilter::FindInterface( const char*  pFilterId, const char* pFilterName, const char* /*pInterfaceId*/ )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	if( !strcmp( (char*)GetFilterId(),		pFilterId ) ||
		!strcmp( (char*)GetFilterName(),	pFilterName ) )
	{
		NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
		return (NX_CV4l2VipFilter*)this;
	}


	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return NULL;
}

//------------------------------------------------------------------------------
NX_CBasePin* NX_CV4l2VipFilter::FindPin( int32_t iDirection, int32_t iIndex )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	NX_CBasePin *pBasePin = NULL;
	if( NX_PIN_OUTPUT == iDirection && 0 == iIndex )
		pBasePin = (NX_CBasePin*)m_pOutputPin;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return pBasePin;
}

//------------------------------------------------------------------------------
void NX_CV4l2VipFilter::GetFilterInfo( NX_FILTER_INFO *pInfo )
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
int32_t NX_CV4l2VipFilter::Run( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );
	int32_t ret = 0;

	if( false == m_bRun )
	{
		if( m_pOutputPin && m_pOutputPin->IsConnected() )
			m_pOutputPin->Active();

		ret = V4l2CamInit();
		if(ret < 0)
		{
			printf("V4l2CameraInit Fail\n");
			return -1;
		}

		m_pOutputPin->AllocateBuffer( MAX_OUTPUT_NUM );

		m_pReleaseQueue = new NX_CQueue();
		m_pReleaseQueue->Reset();


		m_bThreadRun = true;

#ifdef ADJUST_THREAD_PRIORITY
		NX_AdjustThreadPriority(&thread_attrs, SCHED_RR, 50);

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
int32_t NX_CV4l2VipFilter::Stop( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	if( true == m_bRun )
	{
		if( m_pOutputPin && m_pOutputPin->IsActive() )
			m_pOutputPin->Inactive();

		m_bThreadRun = false;
		m_pOutputPin->ResetSignal();
		pthread_join( m_hThread, NULL );

#ifdef ADJUST_THREAD_PRIORITY
		NX_ThreadAttributeDestroy(&thread_attrs);
#endif
	}

	if( m_pV4l2Camera )
	{
		m_pV4l2Camera->Deinit();
		delete m_pV4l2Camera;
		m_pV4l2Camera = NULL;
	}

	m_pOutputPin->FreeBuffer();

	if( m_pReleaseQueue ) {
		delete m_pReleaseQueue;
		m_pReleaseQueue = NULL;
	}
	m_bRun = false;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CV4l2VipFilter::Pause( int32_t mbPause )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	m_bPause = mbPause;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
#ifdef ANDROID_SURF_RENDERING
int32_t NX_CV4l2VipFilter::SetConfig( NX_MEDIA_INFO *pInfo , NX_CAndroidRenderer* pAndroidRender)
#else
int32_t NX_CV4l2VipFilter::SetConfig( NX_MEDIA_INFO *pInfo )
#endif
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	if( pInfo->iMediaType != NX_TYPE_VIDEO )
	{
		NxDbgMsg( NX_DBG_WARN, "Fail, MediaType missmatch. ( Require MediaType : NX_TYPE_VIDEO )\n" );
		NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	if( (pInfo->iWidth == 0) || (pInfo->iHeight == 0) )
	{
		NxDbgMsg( NX_DBG_WARN, "Fail, Video Width(%d) / Video Height(%d)\n", pInfo->iWidth, pInfo->iHeight );
		NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	NX_MEDIA_INFO info;
	memset( &info, 0x00, sizeof(info) );

	info.iMediaType 	= pInfo->iMediaType;
	info.iCodec			= pInfo->iCodec;
	info.iBitrate		= pInfo->iBitrate;
	info.iContainerType	= pInfo->iContainerType;
	info.iModule		= pInfo->iModule;
	info.iSensorId		= pInfo->iSensorId;
	info.bInterlace		= pInfo->bInterlace;
	info.iWidth			= pInfo->iWidth;
	info.iHeight		= pInfo->iHeight;
	info.iFpsNum		= pInfo->iFpsNum;
	info.iFpsDen		= pInfo->iFpsDen;
	info.iNumPlane		= pInfo->iNumPlane;
	info.iCropX			= pInfo->iCropX;
	info.iCropY			= pInfo->iCropY;
	info.iCropWidth		= pInfo->iCropWidth;
	info.iCropHeight	= pInfo->iCropHeight;
	info.iOutWidth		= pInfo->iOutWidth;
	info.iOutHeight		= pInfo->iOutHeight;
	info.fEdgeParam		= pInfo->fEdgeParam;

	info.iFormat		= NX_FORMAT_YUV;
	info.pSeqData		= NULL;
	info.iSeqSize		= 0;

#ifdef ANDROID_SURF_RENDERING
	m_pAndroidRender = pAndroidRender;
#endif
	SetMediaInfo( &info );

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CV4l2VipFilter::Capture( int32_t iQuality )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	m_bCapture = true;
	m_iCaptureQuality = iQuality;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}


//------------------------------------------------------------------------------
void NX_CV4l2VipFilter::RegFileNameCallback( int32_t (*cbFunc)(uint8_t*, uint32_t) )
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

void NX_CV4l2VipFilter::SetDeviceFD(int32_t cam_dev_fd, int32_t mem_dev_fd)
{
	m_CamDevFd = cam_dev_fd;
	m_MemDevFd = mem_dev_fd;
}

#ifndef ALLINEDN
#define ALLINEDN(X,N)	( (X+N-1) & (~(N-1)) )
#endif

//------------------------------------------------------------------------------
int32_t NX_CV4l2VipFilter::Init( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	int32_t allocWidth;

	NX_MEDIA_INFO *pInfo;

	m_pOutputPin->GetMediaInfo( &pInfo );

	memset( &info, 0x00, sizeof(info) );

	info.iModule		= pInfo->iModule;
	info.iSensorId		= pInfo->iSensorId;

	info.bInterlace 	= pInfo->bInterlace;

	info.iWidth			= pInfo->iWidth;
	info.iHeight		= pInfo->iHeight;
	info.iFpsNum		= pInfo->iFpsNum;
	info.iFpsDen		= 1;

	info.iNumPlane		= 1;
	pInfo->iNumPlane = 1;

	info.iCropX			= 0;
	info.iCropY			= 0;
	info.iCropWidth		= pInfo->iWidth;
	info.iCropHeight	= pInfo->iHeight;

	info.iOutWidth		= pInfo->iWidth;
	info.iOutHeight		= pInfo->iHeight;

	info.iCamDevFd		= m_CamDevFd;

	if(info.bInterlace == true)
		allocWidth = ALLINEDN(pInfo->iWidth, 128);
	else
 		allocWidth = ALLINEDN(pInfo->iWidth, 64);

#ifdef ANDROID_SURF_RENDERING
	if(m_pAndroidRender == NULL)
#endif
	{
		m_hVideoMemory = (NX_VID_MEMORY_HANDLE*)malloc( sizeof(NX_VID_MEMORY_HANDLE) * NUM_BUFFER );

		for( int32_t i = 0; i < NUM_BUFFER; i++ )
		{
			#ifdef ANDROID_SURF_RENDERING
			m_hVideoMemory[i] = NX_AllocateVideoMemory(m_MemDevFd, allocWidth, pInfo->iHeight, pInfo->iNumPlane, DRM_FORMAT_YUV420, 512 , 0);
			#else
			m_hVideoMemory[i] = NX_AllocateVideoMemory(m_MemDevFd, allocWidth, pInfo->iHeight, pInfo->iNumPlane, DRM_FORMAT_YUV420, 512 , NEXELL_BO_DMA_CACHEABLE);
			#endif

			if(info.iNumPlane == 1)
			{
				m_hVideoMemory[i]->stride[1] = ALLINEDN((m_hVideoMemory[i]->stride[0] >> 1), 16);
				m_hVideoMemory[i]->stride[2] = ALLINEDN((m_hVideoMemory[i]->stride[0] >> 1), 16);
			}

			NX_MapVideoMemory(m_hVideoMemory[i]);

			//memset(m_hVideoMemory[i]->pBuffer[0], 0x0, m_hVideoMemory[i]->size[0]);
		}

	}
#ifdef ANDROID_SURF_RENDERING
	else
	{
		m_pAndroidRender->GetBuffers(NUM_BUFFER, allocWidth, info.iHeight, &m_hVideoMemory);

		for (int32_t i = 0; i < NUM_BUFFER; i++)
		{
			m_pV4l2Camera->AddVideoMemory(m_hVideoMemory[i]);
		}

		if( 0 > m_pV4l2Camera->Init( &info ) )
		{
			delete m_pV4l2Camera;
			m_pV4l2Camera = NULL;

			NxDbgMsg( NX_DBG_ERR, "Fail, V4l2Camera Init().\n");
			NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
			return -1;
		}
	}
#endif


	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

int32_t NX_CV4l2VipFilter::V4l2CamInit()
{
	m_pV4l2Camera = new NX_CV4l2Camera();

	for( int32_t i=0; i<NUM_BUFFER ; i++ )
	{
		m_pV4l2Camera->AddVideoMemory(m_hVideoMemory[i]);
	}

	if( 0 > m_pV4l2Camera->Init( &info ))
	{
		delete m_pV4l2Camera;
		m_pV4l2Camera = NULL;

		NxDbgMsg( NX_DBG_ERR, "Fail, V4l2Camera Init().\n");
		NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CV4l2VipFilter::Deinit( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

#ifdef ANDROID_SURF_RENDERING
	if(m_hVideoMemory )
	{
		for( int32_t i = 0; i < MAX_OUTPUT_NUM; i++ )
		{
			if( m_hVideoMemory[i] )
			{
				if(m_pAndroidRender == NULL)
					NX_FreeVideoMemory( m_hVideoMemory[i] );

				m_hVideoMemory[i] = NULL;
			}
		}
		if(m_pAndroidRender == NULL)
			free( m_hVideoMemory );

		m_hVideoMemory = NULL;
	}
#else
	if(m_hVideoMemory )
	{
		for( int32_t i = 0; i < MAX_OUTPUT_NUM; i++ )
		{
			if( m_hVideoMemory[i] )
			{
				NX_FreeVideoMemory( m_hVideoMemory[i] );
				m_hVideoMemory[i] = NULL;
			}
		}
		free( m_hVideoMemory );
		m_hVideoMemory = NULL;
	}
#endif

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CV4l2VipFilter::ReleaseBuffer( void *pBuf )
{
	return m_pReleaseQueue->Push( pBuf );
}

void NX_CV4l2VipFilter::VipQueueBuffer( int32_t bufIndex )
{
	m_pReleaseQueue->Push( bufIndex );
}


//------------------------------------------------------------------------------
void NX_CV4l2VipFilter::ThreadProc( void )
{
	int32_t bufIdx = -1;

#if DISPLAY_FPS
	double iStartTime = 0;
	double iEndTime = 0;
	double iSumTime = 0;
	int32_t iDspCnt	= 0;
	int32_t iGatheringCnt = 100;
	iStartTime = now_ms();
#endif

	while( m_bThreadRun )
	{

		NX_VID_MEMORY_INFO *pBuf = NULL;

		m_pV4l2Camera->DequeueBuffer(&bufIdx);

		{
			pBuf = (NX_VID_MEMORY_INFO*) m_hVideoMemory[bufIdx];

			preview_idx = bufIdx;


	#if DISPLAY_FPS
			iDspCnt++;
			if( iDspCnt == iGatheringCnt ) {
				iEndTime = now_ms();
				iSumTime = iEndTime - iStartTime;
				NxDbgMsg(NX_DBG_INFO, "FPS = %f fps, Capture Period = %f mSec\n", 1000./(double)(iSumTime / iGatheringCnt) , (double)(iSumTime / iGatheringCnt) );
				iSumTime = 0;
				iDspCnt = 0;
				iStartTime = now_ms();
			}

	#endif

			NX_CSample *pSample = NULL;
			if( 0 > m_pOutputPin->GetDeliverySample( &pSample ) ) {
				NxDbgMsg( NX_DBG_WARN, "Fail, GetDeliverySample().\n" );
				continue;
			}

			if( NULL == pSample ) {
				NxDbgMsg( NX_DBG_WARN, "Fail, Sample is NULL.\n" );
				continue;
			}

			pSample->SetIndex(bufIdx);
			pSample->SetBuffer( (void*)pBuf, sizeof(NX_VID_MEMORY_INFO) );
			pSample->SetActualBufSize( sizeof(NX_VID_MEMORY_INFO) );

			pSample->SetTimeStamp( m_pRefClock ? m_pRefClock->GetRefTime() : NxGetSystemTick() );


			if( 0 > m_pOutputPin->Deliver( pSample ) ) {
				NxDbgMsg( NX_DBG_WARN, "Fail, Deliver().\n" );
			}

			pSample->Unlock();

			while( m_pReleaseQueue->GetCount() > 0 )
			{
				int32_t index;
				m_pReleaseQueue->Pop(&index);
				m_pV4l2Camera->QueueBuffer(index);
			}
		}

	}

}


//------------------------------------------------------------------------------
void *NX_CV4l2VipFilter::ThreadStub( void *pObj )
{
	if( NULL != pObj ) {
		((NX_CV4l2VipFilter*)pObj)->ThreadProc();
	}

	return (void*)0xDEADDEAD;
}

//------------------------------------------------------------------------------
void NX_CV4l2VipFilter::GetResolution(int32_t type, int32_t module, int32_t *width, int32_t *height)
{
	m_pV4l2Camera->GetResolution(type, module, width, height);
}


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CV4l2VipOutputPin
//
#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG	"[NX_CV4l2VipOutputPin]"
#include "NX_DbgMsg.h"

//------------------------------------------------------------------------------
NX_CV4l2VipOutputPin::NX_CV4l2VipOutputPin()
	: m_pSampleQueue( NULL )
	, m_pSemQueue( NULL )
	, m_iNumOfBuffer( 0 )
{

}

//------------------------------------------------------------------------------
NX_CV4l2VipOutputPin::~NX_CV4l2VipOutputPin()
{
	FreeBuffer();
}

//------------------------------------------------------------------------------
int32_t NX_CV4l2VipOutputPin::ReleaseSample( NX_CSample *pSample )
{
	if( NULL == pSample )
		return -1;

	((NX_CV4l2VipFilter*)m_pOwnerFilter)->VipQueueBuffer( pSample->GetIndex() );
	pSample->SetBuffer( NULL, 0 );
	m_pSampleQueue->PushSample( pSample );
	m_pSemQueue->Post();

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CV4l2VipOutputPin::GetDeliverySample( NX_CSample **ppSample )
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
int32_t NX_CV4l2VipOutputPin::AllocateBuffer( int32_t iNumOfBuffer )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	m_pSampleQueue = new NX_CSampleQueue( iNumOfBuffer );
	m_pSampleQueue->ResetValue();

	m_pSemQueue = new NX_CSemaphore( iNumOfBuffer - 1, iNumOfBuffer - 1 );
	m_pSemQueue->ResetValue();

	for( int32_t i = 0; i < iNumOfBuffer; i++ )
	{
		NX_CSample *pSample = new NX_CSample();
		pSample->SetOwner( (NX_CBaseOutputPin*)this );
		m_pSampleQueue->PushSample( (NX_CSample*)pSample );
	}

	m_iNumOfBuffer = iNumOfBuffer;
	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
void NX_CV4l2VipOutputPin::FreeBuffer( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	if( m_pSampleQueue )
	{
		for( int32_t i = 0; i < m_iNumOfBuffer; i++ )
		{
			NX_CSample *pSample = NULL;
			m_pSampleQueue->PopSample( &pSample );

			if( pSample ) delete pSample;
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
void NX_CV4l2VipOutputPin::ResetSignal( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	if( m_pSemQueue )
		m_pSemQueue->ResetSignal();

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
}


#if 0
//------------------------------------------------------------------------------
static int32_t GetChromaSize( NX_VID_MEMORY_INFO *pInMemory, int32_t *iWidth, int32_t *iHeight )
{
	if( NULL == pInMemory )
	{
		printf("Fail, Input Memory. ( %p )\n", pInMemory );
		return -1;
	}

	int32_t iChromaWidth  = 0;
	int32_t iChromaHeight = 0;

	switch( pInMemory->format )
	{
	case V4L2_PIX_FMT_GREY:
		iChromaWidth  = 0;
		iChromaHeight = 0;
		break;

	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_YUV420M:
	case V4L2_PIX_FMT_YVU420:
	case V4L2_PIX_FMT_YVU420M:
	//case V4L2_PIX_FMT_NV12:
	//case V4L2_PIX_FMT_NV12M:
	//case V4L2_PIX_FMT_NV21:
	//case V4L2_PIX_FMT_NV21M:
		iChromaWidth  = pInMemory->width >> 1;
		iChromaHeight = pInMemory->height >> 1;
		break;

	// case V4L2_PIX_FMT_YUV422P:
	// case V4L2_PIX_FMT_YUV422M:
	// case V4L2_PIX_FMT_NV16:
	// case V4L2_PIX_FMT_NV16M:
	// case V4L2_PIX_FMT_NV61:
	// case V4L2_PIX_FMT_NV61M:
	// 	iChromaWidth  = pInMemory->width >> 1;
	// 	iChromaHeight = pInMemory->height;
	// 	break;

	// case V4L2_PIX_FMT_YUV444:
	// case V4L2_PIX_FMT_YUV444M:
	// case V4L2_PIX_FMT_NV24:
	// case V4L2_PIX_FMT_NV24M:
	// case V4L2_PIX_FMT_NV42:
	// case V4L2_PIX_FMT_NV42M:
	// 	iChromaWidth  = pInMemory->width;
	// 	iChromaHeight = pInMemory->height;
	// 	break;

	default:
		break;
	}

	*iWidth  = iChromaWidth;
	*iHeight = iChromaHeight;

	return 0;
}


//------------------------------------------------------------------------------
int32_t NX_V4l2DumpMemory( NX_VID_MEMORY_INFO *pInMemory, const char *pOutFile )
{
	if( NULL == pInMemory )
	{
		printf("Fail, Input Memory. ( %p )\n", pInMemory );
		return -1;
	}

	if( NULL == pOutFile )
	{
		printf("Fail, Output File.\n" );
		return -1;
	}

	if( NULL == pInMemory->pBuffer[0] )
	{
		if( 0 > NX_MapVideoMemory( pInMemory ) )
		{
			printf("Fail, NX_MapVideoMemory().\n");
			return -1;
		}
	}

	FILE *pFile = fopen( pOutFile, "wb" );
	if( NULL == pFile )
	{
		printf("Fail, fopen(). ( %s )\n", pOutFile );
		return -1;
	}

	int32_t iLumaWidth, iLumaHeight;
	int32_t iChromaWidth, iChromaHeight;

	iLumaWidth  = pInMemory->width;
	iLumaHeight = pInMemory->height;
	GetChromaSize( pInMemory, &iChromaWidth, &iChromaHeight );

	// Save Luma
	{
		uint8_t *pPtr = (uint8_t*)pInMemory->pBuffer[0];
		for( int32_t h = 0; h < iLumaHeight; h++ )
		{
			fwrite( pPtr, 1, iLumaWidth, pFile );
			pPtr += pInMemory->stride[0];
		}
	}

	// Save Chroma
	for( int32_t i = 1; i < pInMemory->planes; i++ )
	{
		uint8_t *pPtr = (uint8_t*)pInMemory->pBuffer[i];
		for( int32_t h = 0; h < iChromaHeight; h++ )
		{
			fwrite( pPtr, 1, iChromaWidth, pFile );
			pPtr += pInMemory->stride[i];
		}
	}

	if( pFile )
	{
		fclose( pFile );
	}

	return 0;
}

#endif