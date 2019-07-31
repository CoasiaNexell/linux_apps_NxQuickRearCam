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
#include "NX_CVideoRenderFilter.h"
#include <drm_mode.h>
#include <drm_fourcc.h>
#define virtual vir
#include <xf86drm.h>
#include <xf86drmMode.h>
#undef virtual

#ifdef ANDROID
#include "dp.h"
#include "dp_common.h"
#endif

#define NX_FILTER_ID		"NX_FILTER_VIDEO_RENDERER"
#define DRM_MODE_OBJECT_PLANE  0xeeeeeeee


#define STREAM_CAPTURE	0

#if STREAM_CAPTURE
FILE *pf_dump;
int32_t frame_cnt;
#endif

// #define NX_DBG_OFF

////////////////////////////////////////////////////////////////////////////////
//
//	NX_CVideoRenderFilter
//
#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_CVideoRenderFilter]"
#include <NX_DbgMsg.h>


#define ROUND_UP_16(num) (((num)+15)&~15)
#define ROUND_UP_32(num) (((num)+31)&~31)

#define DISPLAY_FPS		0
#if DISPLAY_FPS
static double now_ms(void)
{
	struct timespec res;
	clock_gettime(CLOCK_REALTIME, &res);
	return 1000.0*res.tv_sec + (double)res.tv_nsec/1e6;
}
#endif
//------------------------------------------------------------------------------
NX_CVideoRenderFilter::NX_CVideoRenderFilter()
	: m_pInputPin( NULL )
	, m_hThread( 0x00 )
	, m_bThreadRun( false )
	, m_bPause( true )
	, m_pPrvSample( NULL )
	, m_plBufferId(0)
	, getFirstFrame(false)
{
	SetFilterId( NX_FILTER_ID );

	NX_PIN_INFO	info;
	m_pInputPin		= new NX_CVideoRenderInputPin();
	info.iIndex 	= m_FilterInfo.iInPinNum;
	info.iDirection	= NX_PIN_INPUT;
	m_pInputPin->SetOwner( this );
	m_pInputPin->SetPinInfo( &info );
	m_FilterInfo.iInPinNum++;


	for(int32_t i=0; i < MAX_INPUT_BUFFER ; i++ )
	{
		m_BufferId[i] = 0;
	}
	m_LastBufferIndex = -1;


}

//------------------------------------------------------------------------------
NX_CVideoRenderFilter::~NX_CVideoRenderFilter()
{
	Deinit();

	if( m_pInputPin )	delete m_pInputPin;
}

//------------------------------------------------------------------------------
void* NX_CVideoRenderFilter::FindInterface( const char*  pFilterId, const char* pFilterName, const char* pInterfaceId )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	if( !strcmp( (char*)GetFilterId(),		pFilterId )		||
		!strcmp( (char*)GetFilterName(),	pFilterName )	||
		!strcmp( pInterfaceId, 				"" ) )
	{
		NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
		return NULL;
	}

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return NULL;
}

//------------------------------------------------------------------------------
NX_CBasePin* NX_CVideoRenderFilter::FindPin( int32_t iDirection, int32_t iIndex )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	NX_CBasePin *pBasePin = NULL;
	if( NX_PIN_INPUT == iDirection && 0 == iIndex )
		pBasePin = (NX_CBasePin*)m_pInputPin;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return pBasePin;
}

//------------------------------------------------------------------------------
void NX_CVideoRenderFilter::GetFilterInfo( NX_FILTER_INFO *pInfo )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	pInfo->pFilterId	= (char*)GetFilterId();
	pInfo->pFilterName	= (char*)GetFilterName();
	pInfo->iInPinNum	= m_FilterInfo.iInPinNum;

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
}

//------------------------------------------------------------------------------
int32_t NX_CVideoRenderFilter::Run( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	if( false == m_bRun )
	{
		if( m_pInputPin )
			m_pInputPin->Active();

		Init();

		m_bThreadRun = true;
		if( 0 > pthread_create( &this->m_hThread, NULL, this->ThreadStub, this ) ) {
			NxDbgMsg( NX_DBG_ERR, "Fail, Create Thread.\n" );
			return -1;
		}

		m_bRun = true;
	}

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CVideoRenderFilter::Stop( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	if( true == m_bRun )
	{
		if( m_pInputPin && m_pInputPin->IsActive() )
			m_pInputPin->Inactive();

		m_bThreadRun = false;
		m_pInputPin->ResetSignal();
		pthread_join( m_hThread, NULL );

		Deinit();

		m_bRun = false;
	}

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CVideoRenderFilter::Pause( int32_t mbPause )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );
	NX_CAutoLock lock( &m_hLock );

	m_bPause = mbPause;

	if(m_bPause == true)
		DspVideoSetPriority(2);
	else
		DspVideoSetPriority(0);

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}


//------------------------------------------------------------------------------
int32_t NX_CVideoRenderFilter::DrmIoctl( int32_t fd, unsigned long request, void *pArg )
{
	int32_t ret;

	do {
		ret = ioctl(fd, request, pArg);
	} while (ret == -1 && (errno == EINTR || errno == EAGAIN));

	return ret;
}

//------------------------------------------------------------------------------
int32_t NX_CVideoRenderFilter::ImportGemFromFlink( int32_t fd, uint32_t flinkName )
{
	struct drm_gem_open arg;

	memset( &arg, 0x00, sizeof( drm_gem_open ) );

	arg.name = flinkName;
	if (DrmIoctl(fd, DRM_IOCTL_GEM_OPEN, &arg)) {
		return -EINVAL;
	}

	return arg.handle;
}


//------------------------------------------------------------------------------
int32_t NX_CVideoRenderFilter::SetPosition( int32_t x, int32_t y, int32_t width, int32_t height )
{
	NX_CAutoLock lock(&m_hDspCtrlLock);
	m_DspInfo.dspDstRect.left = x;
	m_DspInfo.dspDstRect.top = y;
	// m_DspInfo.dspDstRect.right = x + width;
	// m_DspInfo.dspDstRect.bottom = y + height;
	m_DspInfo.dspDstRect.right = width;
	m_DspInfo.dspDstRect.bottom = height;


	if( m_BufferId[m_LastBufferIndex]!=0 && m_LastBufferIndex >= 0 )
	{
		int32_t err = drmModeSetPlane( m_DspInfo.drmFd, m_DspInfo.planeId, m_DspInfo.ctrlId, m_BufferId[m_LastBufferIndex], 0,
				m_DspInfo.dspDstRect.left, m_DspInfo.dspDstRect.top, m_DspInfo.dspDstRect.right, m_DspInfo.dspDstRect.bottom,
				m_DspInfo.dspSrcRect.left << 16, m_DspInfo.dspSrcRect.top << 16, m_DspInfo.dspSrcRect.right << 16, m_DspInfo.dspSrcRect.bottom << 16 );
		return err;
	}
	return 1;
}

#ifdef ANDROID_SURF_RENDERING
int32_t NX_CVideoRenderFilter::SetConfig( NX_DISPLAY_INFO *pDspInfo, NX_CAndroidRenderer* pAndroidRender )
#else
int32_t NX_CVideoRenderFilter::SetConfig( NX_DISPLAY_INFO *pDspInfo )
#endif
{

	NxDbgMsg( NX_DBG_INFO, "%s()++\n", __FUNCTION__ );
	memset(&m_DspInfo, 0x00, sizeof(m_DspInfo));

	m_DspInfo.connectorId			= pDspInfo->connectorId;
	m_DspInfo.planeId				= pDspInfo->planeId;
	m_DspInfo.ctrlId				= pDspInfo->ctrlId;
	m_DspInfo.width				= pDspInfo->width;
	m_DspInfo.height				= pDspInfo->height;
	m_DspInfo.stride				= 0;
	m_DspInfo.drmFormat			= pDspInfo->drmFormat;
	m_DspInfo.numPlane				= 1;//MEM_NUM_PLANE;
	m_DspInfo.dspSrcRect.left		= pDspInfo->dspSrcRect.left;
	m_DspInfo.dspSrcRect.top		= pDspInfo->dspSrcRect.top;
	m_DspInfo.dspSrcRect.right		= pDspInfo->dspSrcRect.right;
	m_DspInfo.dspSrcRect.bottom		= pDspInfo->dspSrcRect.bottom;
	m_DspInfo.dspDstRect.left		= pDspInfo->dspDstRect.left;
	m_DspInfo.dspDstRect.top		= pDspInfo->dspDstRect.top;
	m_DspInfo.dspDstRect.right		= pDspInfo->dspDstRect.right;
	m_DspInfo.dspDstRect.bottom		= pDspInfo->dspDstRect.bottom;
	m_DspInfo.pglEnable			= pDspInfo->pglEnable;
	m_DspInfo.lcd_width			= pDspInfo->lcd_width;
	m_DspInfo.lcd_height			= pDspInfo->lcd_height;
	m_DspInfo.setCrtc			= pDspInfo->setCrtc;

#ifdef ANDROID_SURF_RENDERING
	m_pAndroidRender = pAndroidRender;
#endif
	NxDbgMsg( NX_DBG_INFO, "%s()--\n", __FUNCTION__ );

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CVideoRenderFilter::Init( /*NX_DISPLAY_INFO *pDspInfo*/ )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	m_pInputPin->AllocateBuffer( MAX_INPUT_NUM );

#ifndef ANDROID_SURF_RENDERING
	m_DspInfo.drmFd = drmOpen( "nexell", NULL );

	if( 0 > m_DspInfo.drmFd )
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, drmOpen().\n" );
		return -1;
	}

	drmSetMaster( m_DspInfo.drmFd );


	if( 0 > drmSetClientCap(m_DspInfo.drmFd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) )
	{
		drmClose( m_DspInfo.drmFd );
		NxDbgMsg( NX_DBG_ERR, "Fail, drmSetClientCap().\n" );
		return -1;
	}

#ifdef ANDROID
{
	if(m_DspInfo.setCrtc == 1)
	{
		drmModeRes *resources = NULL;
		drmModeModeInfo* mode = NULL;
		drmModeConnector *connector = NULL;
		int i, area;

		resources = drmModeGetResources(m_DspInfo.drmFd);

		drm_device = dp_device_open(m_DspInfo.drmFd);

		struct dp_plane *plane;
		uint32_t format;
		int err;
		int d_idx = 0, p_idx = 1;
		default_fb = NULL;

		plane = dp_device_find_plane_by_index(drm_device, d_idx, p_idx);
		if (!plane) {
			printf("no overlay plane found\n");
			return -1;
		}

		err = dp_plane_supports_format(plane, DRM_FORMAT_ARGB8888);
		if (!err) {
			printf("fail : no matching format found\n");
			return -1;
		}else
		{
			format = DRM_FORMAT_ARGB8888;
		}

		default_fb = dp_framebuffer_create(drm_device, format, m_DspInfo.lcd_width, m_DspInfo.lcd_height, 0);
		if (!default_fb) {
			printf("fail : framebuffer create Fail \n");
			return -1;
		}

		err = dp_framebuffer_addfb2(default_fb);

		if (err < 0) {
			printf("fail : framebuffer add Fail \n");
			if (default_fb)
				dp_framebuffer_free(default_fb);
			return -1;
		}

		for (i = 0; i < resources->count_connectors; i++) {
			connector = drmModeGetConnector(m_DspInfo.drmFd, resources->connectors[i]);
			if ((connector->connector_id == m_DspInfo.connectorId) && (connector->connection == DRM_MODE_CONNECTED)) {
				/* it's connected, let's use this! */
				break;
			}

			drmModeFreeConnector(connector);
			connector = NULL;
		}

		if (!connector) {
			/* we could be fancy and listen for hotplug events and wait for
			* a connector..
			*/
			printf("no connected connector!\n");
			return -1;
		}

		for (i = 0, area = 0; i < connector->count_modes; i++) {
			drmModeModeInfo *current_mode = &connector->modes[i];

			if (current_mode->type & DRM_MODE_TYPE_PREFERRED) {
				mode = current_mode;
			}

			int current_area = current_mode->hdisplay * current_mode->vdisplay;
			if (current_area > area) {
				mode = current_mode;
				area = current_area;
			}
		}

		int ret = drmModeSetCrtc(m_DspInfo.drmFd, m_DspInfo.ctrlId, default_fb->id, 0, 0,
				&m_DspInfo.connectorId, 1, mode);
	}
}
#endif


#ifdef UI_OVERLAY_APP
	DspVideoSetPriority(2);
#else
	if(m_DspInfo.pglEnable == 1)
		DspVideoSetPriority(1);
	else
		DspVideoSetPriority(0);	
#endif

#else
	DspVideoSetPriority(2);
#endif
	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CVideoRenderFilter::Deinit( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	m_pInputPin->Flush();
	m_pInputPin->FreeBuffer();

#ifndef ANDROID_SURF_RENDERING
	if( m_DspInfo.drmFd > 0 )
	{
		// clean up object here
		for( int32_t i = 0; i < MAX_INPUT_BUFFER; i++ )
		{
			if( 0 < m_BufferId[i] )
			{
				drmModeRmFB( m_DspInfo.drmFd, m_BufferId[i] );
				m_BufferId[i] = 0;
			}
		}
#ifdef ANDROID
		if(default_fb)
		{
			dp_framebuffer_delfb2(default_fb);
			dp_framebuffer_free(default_fb);
		}
#endif
		if( 0 <= m_DspInfo.drmFd )
		{
			drmClose( m_DspInfo.drmFd );
			m_DspInfo.drmFd = -1;
		}
		memset( &m_DspInfo, 0, sizeof(m_DspInfo) );
		m_DspInfo.drmFd = -1;
	}
	DspVideoSetPriority(2);
#endif
	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CVideoRenderFilter::Render( NX_CSample *pSample )
{
	NX_VID_MEMORY_INFO *pImg = NULL;
	int32_t bufferIdx;
	int32_t err;

	int32_t iBufSize = 0;

	if( 0 > pSample->GetBuffer( (void**)&pImg, &iBufSize) ) {
		NxDbgMsg( NX_DBG_ERR, "Fail, GetBuffer().\n" );
		return -1;
	}
	bufferIdx = pSample->GetIndex();

#ifndef ANDROID_SURF_RENDERING

	if(m_BufferId[bufferIdx] == 0)
	{
		int32_t i = 0;
		uint32_t handles[4] = { 0, };
		uint32_t pitches[4] = { 0, };
		uint32_t offsets[4] = { 0, };
		uint32_t offset = 0;
		uint32_t strideWidth[3] = { 0, };
		uint32_t strideHeight[3] = { 0, };

		{
			strideWidth[0] = pImg->stride[0];
			strideWidth[1] = pImg->stride[1];
			strideWidth[2] = pImg->stride[2];
			strideHeight[0] = ROUND_UP_16(pImg->height);
			strideHeight[1] = ROUND_UP_16(pImg->height >> 1);
			strideHeight[2] = strideHeight[1];
		}

		//	FIXME: Hardcoded "3"
		for( i = 0; i <3 /*pImg->planes*/; i++ )
		{
			handles[i] = (pImg->planes == 1) ?
				ImportGemFromFlink( m_DspInfo.drmFd, pImg->flink[0] ) :
				ImportGemFromFlink( m_DspInfo.drmFd, pImg->flink[i] );
			pitches[i] = strideWidth[i];
			offsets[i] = offset;
			offset += ( (pImg->planes == 1) ? (strideWidth[i] * strideHeight[i]) : 0 );
		}

		err = drmModeAddFB2( m_DspInfo.drmFd, pImg->width, pImg->height,
		 		m_DspInfo.drmFormat, handles, pitches, offsets, &m_BufferId[bufferIdx], 0 );


		if( 0 > err )
		{
			NxDbgMsg( NX_DBG_ERR, "Fail, drmModeAddFB2() !!!. err = %d\n", err );
			//return -1;
		}


		//NxDbgMsg( NX_DBG_VBS, "Resol(%dx%d), pitch(%d,%d,%d,%d)\n",
		//	pImg->width, pImg->height, pitches[0], pitches[1], pitches[2], pitches[3]);

	}

	{
#if STREAM_CAPTURE

		{
			uint32_t strideWidth[3] = { 0, };
			uint32_t strideHeight[3] = { 0, };
			strideWidth[0] = pImg->stride[0];
			strideWidth[1] = pImg->stride[1];
			strideWidth[2] = pImg->stride[2];
			strideHeight[0] = ROUND_UP_16(pImg->height);
			strideHeight[1] = ROUND_UP_16(pImg->height >> 1);
			strideHeight[2] = strideHeight[1];

			printf("frame_cnt = %d\n", frame_cnt);

			if(frame_cnt >= 200 && frame_cnt<1000)
			{
				uint8_t *temp =(uint8_t*) pImg->pBuffer[0];

				for(int i = 0; i<pImg->height; i++)
				{
					fwrite(temp, sizeof(uint8_t), /*pImg->width*/704, pf_dump);
					temp += pImg->stride[0];
				}

				for(int i=0; i<pImg->height/2; i++)
				{
					fwrite(temp, sizeof(uint8_t), /*pImg->width/2*/352, pf_dump);
					temp += pImg->stride[1];
				}

				for(int i=0; i<pImg->height/2; i++)
				{
					fwrite(temp, sizeof(uint8_t), /*pImg->width/2*/352, pf_dump);
					temp += pImg->stride[2];
				}

			}
			frame_cnt ++;

			if(frame_cnt == 1000)
				fclose(pf_dump);
		}

#endif

		NX_SyncVideoMemory(pImg->drmFd, pImg->gemFd[0], pImg->size[0]);

		NX_CAutoLock lock(&m_hDspCtrlLock);


		err = drmModeSetPlane( m_DspInfo.drmFd, m_DspInfo.planeId, m_DspInfo.ctrlId, m_BufferId[bufferIdx], 0,
						m_DspInfo.dspDstRect.left, m_DspInfo.dspDstRect.top, m_DspInfo.dspDstRect.right, m_DspInfo.dspDstRect.bottom,
				m_DspInfo.dspSrcRect.left << 16, m_DspInfo.dspSrcRect.top << 16, m_DspInfo.dspSrcRect.right << 16, m_DspInfo.dspSrcRect.bottom << 16 );

		if(getFirstFrame == false)
		{
			printf("[QuickRearCam] First Frame Display\n");
			getFirstFrame = true;
		}

		m_LastBufferIndex = bufferIdx;

	}

	if( 0 > err )
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, drmModeSetPlane() !!!.\n");
		return -1;
	}
#else
	m_pAndroidRender->QueueBuffer(bufferIdx);

	if(m_pPrvSample != NULL)
		m_pAndroidRender->DequeueBuffer();

	m_pPrvSample = pSample;

#endif

	return 0;
}



//------------------------------------------------------------------------------
void NX_CVideoRenderFilter::ThreadProc( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	NX_CSample *pSample = NULL;

#if STREAM_CAPTURE
	pf_dump = fopen("/sdcard/interlace_test/dump.yuv", "wb");
	frame_cnt = 0;
#endif

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
		if( 0 > m_pInputPin->GetSample( &pSample) ) {
			NxDbgMsg( NX_DBG_WARN, "Fail, GetSample().\n" );
			continue;
		}

		if( NULL == pSample ) {
			NxDbgMsg( NX_DBG_WARN, "Fail, Sample is NULL.\n" );
			continue;
		}

		{
			Render( pSample );

			pSample->Unlock();

		}
#if DISPLAY_FPS
		iDspCnt++;
		if( iDspCnt == iGatheringCnt ) {
			iEndTime = now_ms();
			iSumTime = iEndTime - iStartTime;
			NxDbgMsg(NX_DBG_INFO, "FPS = %f fps, Display Period = %f mSec\n", 1000./(double)(iSumTime / iGatheringCnt) , (double)(iSumTime / iGatheringCnt) );
			iSumTime = 0;
			iDspCnt = 0;
			iStartTime = now_ms();
		}

#endif
	}

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
}

//------------------------------------------------------------------------------
void *NX_CVideoRenderFilter::ThreadStub( void *pObj )
{
	if( NULL != pObj ) {
		((NX_CVideoRenderFilter*)pObj)->ThreadProc();
	}

	return (void*)0xDEADDEAD;
}

int32_t NX_CVideoRenderFilter::DspVideoSetPriority(int32_t priority)
{
	char *prop_name = "video-priority";
	int res = -1;
	unsigned int i = 0;
	int prop_id = -1;
	drmModeObjectPropertiesPtr props;

	props = drmModeObjectGetProperties(m_DspInfo.drmFd, m_DspInfo.planeId,	DRM_MODE_OBJECT_PLANE);

	if (props) {
		for (i = 0; i < props->count_props; i++) {
			drmModePropertyPtr prop;
			prop = drmModeGetProperty(m_DspInfo.drmFd, props->props[i]);

			if (prop) {
				if (!strncmp(prop->name, prop_name, DRM_PROP_NAME_LEN))
				{
					prop_id = prop->prop_id;
					drmModeFreeProperty(prop);
					break;
				}
				drmModeFreeProperty(prop);
			}
		}
		drmModeFreeObjectProperties(props);
	}

	if(prop_id >= 0)
		res = drmModeObjectSetProperty(m_DspInfo.drmFd, m_DspInfo.planeId,	DRM_MODE_OBJECT_PLANE, prop_id, priority);

	return res;
}


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CVideoRenderInputPin
//
#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_CVideoRenderInputPin]"
#include <NX_DbgMsg.h>

//------------------------------------------------------------------------------
NX_CVideoRenderInputPin::NX_CVideoRenderInputPin()
{

}

//------------------------------------------------------------------------------
NX_CVideoRenderInputPin::~NX_CVideoRenderInputPin()
{

}

//------------------------------------------------------------------------------
int32_t NX_CVideoRenderInputPin::Receive( NX_CSample *pSample )
{
	if( NULL == m_pSampleQueue || false == IsActive() )
		return -1;

	pSample->Lock();
	m_pSampleQueue->PushSample( pSample );
	m_pSemQueue->Post();

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CVideoRenderInputPin::GetSample( NX_CSample **ppSample )
{
	*ppSample = NULL;

	if( NULL == m_pSampleQueue )
		return -1;

	if( 0 > m_pSemQueue->Pend() )
		return -1;

	return m_pSampleQueue->PopSample( ppSample );
}

//------------------------------------------------------------------------------
int32_t NX_CVideoRenderInputPin::Flush( void )
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
int32_t NX_CVideoRenderInputPin::PinNegotiation( NX_CBaseOutputPin *pOutPin )
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
int32_t NX_CVideoRenderInputPin::AllocateBuffer( int32_t iNumOfBuffer )
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
void NX_CVideoRenderInputPin::FreeBuffer( void )
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
void NX_CVideoRenderInputPin::ResetSignal( void )
{
	NxDbgMsg( NX_DBG_VBS, "%s()++\n", __FUNCTION__ );

	if( m_pSemQueue )
		m_pSemQueue->ResetSignal();

	NxDbgMsg( NX_DBG_VBS, "%s()--\n", __FUNCTION__ );
}

