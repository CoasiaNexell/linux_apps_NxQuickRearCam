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

#ifdef  ANDROID_SURF_RENDERING

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/GraphicBuffer.h>

// #include <gralloc_priv.h>	//	private_handle_t
// #include <nexell_format.h>	//	ALIGN

// #include <nx_fourcc.h>

#include "NX_CAndroidRenderer.h"

// #define NX_DBG_OFF

#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG	"[NX_CAndroidRenderer]"
#include "NX_DbgMsg.h"

//------------------------------------------------------------------------------
NX_CAndroidRenderer::NX_CAndroidRenderer( ANativeWindow *pYuvWindow )
	: m_iNumBuffer( 0 )
{
	for( int32_t i = 0 ; i<MAX_BUFFER_NUM; i++ )
	{
		m_pYuvANBuffer[i]	= NULL;
		m_hMemoryHandle[i]	= NULL;
	}
	m_pYuvWindow = pYuvWindow;
}

//------------------------------------------------------------------------------
NX_CAndroidRenderer::NX_CAndroidRenderer( int32_t iWidth, int32_t iHeight )
	: m_iNumBuffer( 0 )
{
	for( int32_t i = 0 ; i<MAX_BUFFER_NUM; i++ )
	{
		m_pYuvANBuffer[i]	= NULL;
		m_hMemoryHandle[i]	= NULL;
	}

	sp<SurfaceComposerClient> surfComClient = new SurfaceComposerClient();
	sp<SurfaceControl> surfaceControl =
		surfComClient->createSurface(String8("YUV Surface"), iWidth, iHeight, HAL_PIXEL_FORMAT_YV12, ISurfaceComposerClient::eFXSurfaceNormal);
	if( surfaceControl == NULL ) {
		NxDbgMsg(NX_DBG_ERR, "Fail, Create SurfaceControl!!!\n");
		exit(-1);
	}

	SurfaceComposerClient::openGlobalTransaction();
	surfaceControl->show();
	surfaceControl->setLayer(99999);
	SurfaceComposerClient::closeGlobalTransaction();

	sp<ANativeWindow> yuvWindow(surfaceControl->getSurface());

	m_SurfaceControl	= surfaceControl;
	m_pYuvWindow 		= yuvWindow.get();
}

//------------------------------------------------------------------------------
NX_CAndroidRenderer::~NX_CAndroidRenderer()
{
	for( int32_t i = 0; i < MAX_BUFFER_NUM; i++)
	{
		if( m_hMemoryHandle[i] )
		{
			NX_UnmapVideoMemory( m_hMemoryHandle[i]);
			free( m_hMemoryHandle[i] );
			m_hMemoryHandle[i] = NULL;
		}
	}

	if( m_pYuvWindow )
	{
		m_pYuvWindow = NULL;
	}
}

//------------------------------------------------------------------------------
int32_t NX_CAndroidRenderer::GetBuffers( int32_t iNumBuf, int32_t iWidth, int32_t iHeight, NX_VID_MEMORY_HANDLE **pMemHandle )
{
	if( iNumBuf > MAX_BUFFER_NUM ) {
		NxDbgMsg(NX_DBG_ERR, "Error : iNumBuf( %d ) is large than MAX_BUFFER_NUM ( %d ).\n", iNumBuf, MAX_BUFFER_NUM );
		return -1;
	}

	int32_t err;
	err = native_window_api_connect( m_pYuvWindow, NATIVE_WINDOW_API_CPU );
	if (err) {
		NxDbgMsg(NX_DBG_ERR, "Error : failed to yuv native_window_api_connect!!! : %d\n", err);
		return -1;
	}

	err = native_window_set_buffer_count( m_pYuvWindow, iNumBuf );
	if (err) {
		NxDbgMsg(NX_DBG_ERR, "Error : failed to yuv native_window_set_buffer_count!!!\n");
		return -1;
	}

	err = native_window_set_scaling_mode( m_pYuvWindow, NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW );
	if (err) {
		NxDbgMsg(NX_DBG_ERR, "Error : failed to yuv native_window_set_scaling_mode!!!\n");
		return -1;
	}

	err = native_window_set_usage( m_pYuvWindow, GRALLOC_USAGE_HW_CAMERA_WRITE );
	if (err) {
		NxDbgMsg(NX_DBG_ERR, "Error : failed to yuv native_window_set_usage!!!\n");
		return -1;
	}

#if 0
	//
	//	deprecated fucntion in Android Nougat.
	//
	err = native_window_set_buffers_geometry( m_pYuvWindow, iImgWidth, iImgHeight, HAL_PIXEL_FORMAT_YV12 );
	if (err) {
		NxDbgMsg(NX_DBG_ERR, "Error : failed to yuv native_window_set_buffers_geometry!!!\n");
		return -1;
	}
#else
	err = native_window_set_buffers_dimensions( m_pYuvWindow, iWidth, iHeight );
	if (err) {
		NxDbgMsg(NX_DBG_ERR, "Error : failed to yuv native_window_set_buffers_dimensions!!!\n");
		return -1;
	}

	err = native_window_set_buffers_format( m_pYuvWindow, HAL_PIXEL_FORMAT_YV12 );
	if (err) {
		NxDbgMsg(NX_DBG_ERR, "Error : failed to yuv native_window_set_buffers_format!!!\n");
		return -1;
	}
#endif

	for (int i = 0; i < iNumBuf; i++)
	{
		err = native_window_dequeue_buffer_and_wait( m_pYuvWindow, &m_pYuvANBuffer[i] );
		if (err) {
			NxDbgMsg(NX_DBG_ERR, "Error : failed to yuv dequeue buffer..!!!\n");
			return -1;
		}

		private_handle_t const *pYuvHandle = reinterpret_cast<private_handle_t const *>(m_pYuvANBuffer[i]->handle);
		m_hMemoryHandle[i] = (NX_VID_MEMORY_HANDLE)calloc(1, sizeof(NX_VID_MEMORY_INFO));
		NX_PrivateHandleToVideoMemory( pYuvHandle, m_hMemoryHandle[i] );
		NX_MapVideoMemory( m_hMemoryHandle[i] );
	}

	*pMemHandle = &m_hMemoryHandle[0];
	m_iNumBuffer = iNumBuf;

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CAndroidRenderer::QueueBuffer( int32_t iIndex )
{
	return m_pYuvWindow->queueBuffer( m_pYuvWindow, m_pYuvANBuffer[iIndex], -1 );
}

//------------------------------------------------------------------------------
int32_t NX_CAndroidRenderer::DequeueBuffer( void )
{
	ANativeWindowBuffer *yuvTempBuffer;
	return native_window_dequeue_buffer_and_wait( m_pYuvWindow, &yuvTempBuffer );
}

//------------------------------------------------------------------------------
int32_t NX_CAndroidRenderer::SetPosition( int32_t iX, int32_t iY, int32_t iWidth, int32_t iHeight )
{
	SurfaceComposerClient::openGlobalTransaction();
	m_SurfaceControl->setPosition( (float)iX, (float)iY );
	m_SurfaceControl->setSize( iWidth, iHeight );
	SurfaceComposerClient::closeGlobalTransaction();

	return 0;
}

#endif //#ifdef  ANDROID_SURF_RENDERING