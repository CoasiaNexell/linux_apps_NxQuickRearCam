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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <media-bus-format.h>
#include <linux/videodev2.h>
#include <videodev2_nxp_media.h>
//#include <nx-drm-allocator.h>
#include "NX_CV4l2Camera.h"

#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG		"[NX_CV4l2Camera]"
#include "NX_DbgMsg.h"

#ifndef ALIGN
#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#endif

//------------------------------------------------------------------------------
NX_CV4l2Camera::NX_CV4l2Camera()
	: m_bInit(0)
	, m_bInterlaced(false)
	, m_iSensorFd(-1)
	, m_iClipperSubdevFd(-1)
	, m_iDecimatorSubdevFd(-1)
	, m_iCsiSubdevFd(-1)
	, m_iClipperVideoFd(-1)
	, m_iDecimatorVideoFd(-1)
	, m_iCurQueuedSize(0)
	, m_iRegBufSize(0)
{
	for(int32_t i = 0; i < MAX_BUF_NUM; i++ )
	{
		m_pMemSlot[i] = NULL;
	}

	pthread_mutex_init( &m_hLock, NULL );
}

//------------------------------------------------------------------------------
NX_CV4l2Camera::~NX_CV4l2Camera()
{
	NxDbgMsg( NX_DBG_VBS, "%s().\n", __func__ );
	Deinit();
	pthread_mutex_destroy( &m_hLock );
}

//------------------------------------------------------------------------------
int32_t NX_CV4l2Camera::V4l2CameraInit()
{
	int32_t ret = 0, i = 0;

	ret = V4l2OpenDevices();
	if( -1 == ret )
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, V4l2OpenDevices().\n" );
		return -1;
	}

	ret = V4l2Link();
	if( -1 == ret )
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, V4l2Link().\n" );
		return -1;
	}

	ret = V4l2SetFormat();
	if( -1 == ret )
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, V4l2SetFormat().\n" );
		return -1;
	}

	ret = nx_v4l2_reqbuf(m_iClipperVideoFd, nx_clipper_video, m_iRegBufSize);
	if (ret)
	{
		NxDbgMsg( NX_DBG_ERR, "failed to reqbuf\n");
		return -1;
	}

	for (i = 0; i < m_iRegBufSize; i++)
	{
#ifndef USE_ION_ALLOCATOR
		//NxDbgMsg( NX_DBG_VBS, "nx_v4l2_qbuf[%d], DmaFd[%d], BufferSize[%d]\n",
		//			i, m_iDmaFds[i][0], m_iBufferSize[i][0]);
		ret = nx_v4l2_qbuf(m_iClipperVideoFd,
							nx_clipper_video, m_iNumPlane, i,
							(int32_t*)&m_iDmaFds[i],
							(int32_t *)&m_iBufferSize[i]);
		if (ret)
		{
			NxDbgMsg( NX_DBG_ERR, "failed to qbuf: index %d\n", i);
			return -1;
		}
#else
		//NxDbgMsg( NX_DBG_VBS, "nx_v4l2_qbuf[%d], sharedFd[%d], BufferSize[%d]\n",
		//			i, sharedFd[i][0], m_iBufferSize[i][0]);
		ret = nx_v4l2_qbuf(m_iClipperVideoFd,
							nx_clipper_video, m_iNumPlane, i,
							(int32_t*)&sharedFd[i],
							(int32_t *)&m_iBufferSize[i]);
		if (ret)
		{
			NxDbgMsg( NX_DBG_ERR, "failed to qbuf: index %d\n", i);
			return -1;
		}
#endif
	}

	ret = nx_v4l2_streamon(m_iClipperVideoFd, nx_clipper_video);
	if (ret)
	{
		NxDbgMsg( NX_DBG_ERR, "failed to streamon");
		return -1;
	}

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CV4l2Camera::V4l2OpenDevices()
{
	// open devices
	m_iSensorFd = nx_v4l2_open_device(m_iSensorId, m_iModule);
	if (m_iSensorFd < 0)
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, nx_v4l2_open_device(%d).\n", m_iSensorId );
		return -1;
	}

	m_iClipperSubdevFd = nx_v4l2_open_device(nx_clipper_subdev, m_iModule);
	if (m_iClipperSubdevFd < 0)
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, nx_v4l2_open_device(nx_clipper_subdev).\n" );
		return -1;
	}

	m_iClipperVideoFd = nx_v4l2_open_device(nx_clipper_video, m_iModule);

	if (m_iClipperVideoFd < 0)
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, nx_v4l2_open_device(clipper_video).\n" );
		return -1;
	}

	m_bIsMiPi = nx_v4l2_is_mipi_camera(m_iModule);
	if (m_bIsMiPi)
	{
		m_iCsiSubdevFd = nx_v4l2_open_device(nx_csi_subdev, m_iModule);
		if (m_iCsiSubdevFd < 0)
		{
			NxDbgMsg( NX_DBG_ERR, "Failed to open mipi csi.\n" );
			return -1;
		}
	}

	NxDbgMsg( NX_DBG_INFO, "[%d] m_iSensorFd = %d, m_iClipperSubdevFd = %d, m_iClipperVideoFd=%d\n",
		m_iModule, m_iSensorFd, m_iClipperSubdevFd, m_iClipperVideoFd);

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CV4l2Camera::V4l2Link()
{
	int32_t ret = 0;

	// link
	ret = nx_v4l2_link(true, m_iModule, nx_clipper_subdev, 1,
						nx_clipper_video, 0);
	if (ret)
	{
		NxDbgMsg( NX_DBG_ERR, "failed to link clipper_sub -> clipper_video.\n" );
		return -1;
	}

	if (m_bIsMiPi)
	{
		ret = nx_v4l2_link(true, m_iModule, nx_sensor_subdev, 0,
						nx_csi_subdev, 0);
		if (ret)
		{
			NxDbgMsg( NX_DBG_ERR, "failed to link sensor -> mipi_csi.\n" );
			return -1;
		}

		ret = nx_v4l2_link(true, m_iModule, nx_csi_subdev, 1,
						nx_clipper_subdev, 0);
		if (ret)
		{
			NxDbgMsg( NX_DBG_ERR, "failed to link mipi_csi -> clipper_sub.\n" );
			return -1;
		}
	}
	else
	{
		ret = nx_v4l2_link(true, m_iModule, nx_sensor_subdev, 0,
						nx_clipper_subdev, 0);
		if (ret)
		{
			NxDbgMsg( NX_DBG_ERR, "failed to link sensor -> clipper_sub.\n" );
			return -1;
		}
	}

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CV4l2Camera::V4l2SetFormat()
{
	int32_t ret = 0;

	NxDbgMsg( NX_DBG_INFO, "m_iWidth=%d, m_iHeight=%d, m_iBusFormat=0x%08x\n", m_iWidth, m_iHeight, m_iBusFormat);

	ret = nx_v4l2_set_format( m_iSensorFd, nx_sensor_subdev, m_iWidth, m_iHeight, m_iBusFormat );
	if (ret)
	{
		NxDbgMsg( NX_DBG_ERR, "failed to set_format for sensor.\n" );
		return -1;
	}

	if( m_bIsMiPi )
	{
		ret = nx_v4l2_set_format( m_iCsiSubdevFd, nx_csi_subdev, m_iWidth, m_iHeight, m_iPixelFormat );
		if (ret)
		{
			NxDbgMsg( NX_DBG_ERR, "failed to nx_v4l2_set_format for mipi_csi.\n" );
			return -1;
		}
	}

	ret = nx_v4l2_set_format(m_iClipperSubdevFd, nx_clipper_subdev, m_iWidth, m_iHeight, m_iBusFormat);
	if (ret)
	{
		NxDbgMsg( NX_DBG_ERR, "failed to nx_v4l2_set_format for nx_clipper_subdev.(m_iClipperSubdevFd=%d)\n",
					m_iClipperSubdevFd );
		return -1;
	}

	if( m_bInterlaced )
	{
		ret = nx_v4l2_set_format_with_field(m_iClipperVideoFd, nx_clipper_video, m_iWidth, m_iHeight, m_iPixelFormat, V4L2_FIELD_INTERLACED);
		if (ret) {
			NxDbgMsg( NX_DBG_ERR, "failed to set_format for clipper video\n");
			return -1;
		}
	}
	else
	{
		ret = nx_v4l2_set_format(m_iClipperVideoFd, nx_clipper_video, m_iWidth, m_iHeight, m_iPixelFormat);
		if (ret)
		{
			NxDbgMsg( NX_DBG_ERR, "failed to nx_v4l2_set_format for nx_clipper_video.\n" );
			return -1;
		}
	}

	if (m_iCropX && m_iCropY && m_iCropWidth && m_iCropHeight)
	{
		ret = nx_v4l2_set_crop(m_iClipperSubdevFd, nx_clipper_subdev,
							m_iCropX, m_iCropY, m_iCropWidth, m_iCropHeight);
		if (ret)
		{
			NxDbgMsg( NX_DBG_ERR, "failed to nx_v4l2_set_crop for nx_clipper_subdev.\n" );
			return -1;
		}
	}
	else
	{
		ret = nx_v4l2_set_crop(m_iClipperSubdevFd, nx_clipper_subdev,
								0, 0, m_iWidth, m_iHeight);
		if (ret)
		{
			NxDbgMsg( NX_DBG_ERR, "failed to set_crop for clipper_subdev.\n" );
			return -1;
		}
	}
	return 0;
}


//------------------------------------------------------------------------------
int32_t NX_CV4l2Camera::Init( NX_VIP_INFO *pInfo )
{
	int32_t ret = 0;

	//
	m_iModule				= pInfo->iModule;
	m_iSensorId				= pInfo->iSensorId;
	m_bInterlaced			= pInfo->bInterlace;
	//
	m_iWidth				= pInfo->iWidth;
	m_iHeight				= pInfo->iHeight;
	m_iCropX				= pInfo->iCropX;
	m_iCropX				= pInfo->iCropY;
	m_iCropWidth			= pInfo->iCropWidth;
	m_iCropHeight			= pInfo->iCropHeight;
	//
	m_iNumPlane				= pInfo->iNumPlane;					// FIX ME!!!!!!!!!!!
	m_iPixelFormat			= V4L2_PIX_FMT_YUV420;
	m_iBusFormat			= MEDIA_BUS_FMT_UYVY8_2X8;  //navi 4418

	//
	m_bIsMiPi				= 0;
	m_iSensorFd				= 0;
	m_iClipperSubdevFd		= 0;
	m_iDecimatorSubdevFd	= 0;
	m_iCsiSubdevFd			= 0;
	m_iClipperVideoFd		= 0;
	m_iDecimatorVideoFd		= 0;

	ret = V4l2CameraInit();
	if( -1 == ret )
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, V4l2Init().\n" );
		return -1;
	}

	m_bInit = 1;

	return 0;
}

//------------------------------------------------------------------------------
void NX_CV4l2Camera::Deinit( void )
{
	if( m_bInit )
	{
		NxDbgMsg( NX_DBG_VBS, "%s().\n", __func__ );
		V4l2Deinit();
		m_iCurQueuedSize= 0;
		for(int32_t i = 0; i < MAX_BUF_NUM; i++ )
		{
			m_pMemSlot[i] = NULL;
#ifndef USE_ION_ALLOCATOR
			memset(m_iDmaFds, 0, sizeof(m_iDmaFds));
			memset(m_iGemFds, 0, sizeof(m_iGemFds));
#endif
			memset(m_iBufferSize, 0, sizeof(m_iBufferSize));
			memset(m_pVirAddr, 0, sizeof(m_pVirAddr));
		}
		m_iRegBufSize = 0;
		m_bInit = 0;
	}
}

//------------------------------------------------------------------------------
void NX_CV4l2Camera::V4l2Deinit()
{
	nx_v4l2_streamoff(m_iClipperVideoFd, nx_clipper_video);
	nx_v4l2_reqbuf(m_iClipperVideoFd, nx_clipper_video, 0);
	nx_v4l2_cleanup();
	if(m_iSensorFd>0)
	{
		close(m_iSensorFd);
		m_iSensorFd = 1;
	}
	if(m_iClipperSubdevFd>0)
	{
		close(m_iClipperSubdevFd);
		m_iClipperSubdevFd = 1;
	}
	if(m_iDecimatorSubdevFd>0)
	{
		close(m_iDecimatorSubdevFd);
		m_iDecimatorSubdevFd = 1;
	}
	if(m_iCsiSubdevFd>0)
	{
		close(m_iCsiSubdevFd);
		m_iCsiSubdevFd = 1;
	}
	if(m_iClipperVideoFd>0)
	{
		close(m_iClipperVideoFd);
		m_iClipperVideoFd = 1;
	}
	if(m_iDecimatorVideoFd>0)
	{
		close(m_iDecimatorVideoFd);
		m_iDecimatorVideoFd = -1;
	}
}

//------------------------------------------------------------------------------
int32_t NX_CV4l2Camera::QueueBuffer( int32_t bufferIndex )
{
	int32_t iRet = 0;

	//	Update Queue Size
	pthread_mutex_lock( &m_hLock );
	if( bufferIndex >= MAX_BUF_NUM )
	{
		pthread_mutex_unlock( &m_hLock );
		return -1;
	}
	m_iCurQueuedSize++;
	pthread_mutex_unlock( &m_hLock );


	iRet = nx_v4l2_qbuf( m_iClipperVideoFd, nx_clipper_video,
						m_iNumPlane,
						bufferIndex,
						#ifndef USE_ION_ALLOCATOR
						(int32_t *)&m_iDmaFds[bufferIndex],
						#else
						(int32_t *)&sharedFd[bufferIndex],
						#endif
						(int32_t *)&m_iBufferSize[bufferIndex]);

	if( 0 > iRet )
	{
		m_pMemSlot[bufferIndex] = NULL;
		NxDbgMsg( NX_DBG_ERR, "Fail, nx_v4l2_qbuf().\n" );
		return iRet;
	}

	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CV4l2Camera::DequeueBuffer( int32_t *pBufferIndex )
{
	int32_t iRet = 0;
	int32_t iSlotIndex = -1;

	pthread_mutex_lock( &m_hLock );

	if( m_iCurQueuedSize < 2 )
	{
		pthread_mutex_unlock( &m_hLock );
		return -1;
	}
	pthread_mutex_unlock( &m_hLock );


	iRet = nx_v4l2_dqbuf(m_iClipperVideoFd, nx_clipper_video, m_iNumPlane, &iSlotIndex);



	if( 0 > iRet )
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, nx_v4l2_dqbuf().\n" );
		return iRet;
	}

	*pBufferIndex = iSlotIndex;
	pthread_mutex_lock( &m_hLock );
	m_iCurQueuedSize--;
	pthread_mutex_unlock( &m_hLock );

	return iRet;
}

//------------------------------------------------------------------------------
int32_t NX_CV4l2Camera::AddVideoMemory( NX_VID_MEMORY_INFO *pVidMem )
{
	int32_t i = 0;

	pthread_mutex_lock( &m_hLock );

	if( m_iCurQueuedSize >= MAX_BUF_NUM )
	{
		pthread_mutex_unlock( &m_hLock );
		return -1;
	}

	for( i = 0; i < MAX_BUF_NUM; i++ )
	{
		if( m_pMemSlot[i] == NULL )
		{
			m_pMemSlot[i] = pVidMem;
#ifndef USE_ION_ALLOCATOR
			for( int32_t j=0 ; j<NX_MAX_PLANES ; j++ )
			{
				m_iDmaFds[i][j] 	= pVidMem->dmaFd[j];
				m_iGemFds[i][j] 	= pVidMem->gemFd[j];
				m_iBufferSize[i][j]	= pVidMem->size[j];
				m_iStride[i][j]		= pVidMem->stride[j];
				m_pVirAddr[i][j]	= pVidMem->pBuffer[j];
			}
#else

			for( int32_t j=0 ; j<NX_MAX_PLANES ; j++ )
			{
				sharedFd[i][j] 	= pVidMem->sharedFd[j];
				m_iBufferSize[i][j]	= pVidMem->size[j];
				m_iStride[i][j] = pVidMem->stride[j];
				m_pVirAddr[i][j]	= pVidMem->pBuffer[j];
			}
#endif
			break;
		}
	}

	if( i == MAX_BUF_NUM )
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, Have no empty slot.\n" );
		pthread_mutex_unlock( &m_hLock );
		return -1;
	}

	m_iRegBufSize++;
	m_iCurQueuedSize ++;

	pthread_mutex_unlock( &m_hLock );

	return 0;
}

