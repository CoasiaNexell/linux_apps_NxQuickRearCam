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


#include <string.h>
#include "NX_CRGBDraw.h"
#include <errno.h>
#include <drm_mode.h>
#include <drm_fourcc.h>
#define virtual vir
#include <xf86drm.h>
#include <xf86drmMode.h>
#undef virtual

#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_RGBDraw]"
#include <NX_DbgMsg.h>

#ifndef ALIGN
#define	ALIGN(X,N)		( (X+N-1) & (~(N-1)) )
#endif

NX_CRGBDraw::NX_CRGBDraw(){

}

NX_CRGBDraw::~NX_CRGBDraw(){

}

void NX_CRGBDraw::AllocBuffer(NX_RGB_DRAW_INFO *pInfo)
{
	int32_t mem_size;

	mem_size = pInfo->m_iDspWidth * pInfo->m_iDspHeight * 4;

	for(int i=0; i<pInfo->m_iNumBuffer; i++)
	{
		pRGBData[i] = NX_AllocateMemory(pInfo->m_MemDevFd, mem_size, pInfo->m_iDspWidth * 4);
		NX_MapMemory(pRGBData[i]);
		m_MemInfo[i].mem_size = ALIGN(mem_size, pInfo->m_iDspWidth * 4);
		m_MemInfo[i].pBuffer = pRGBData[i]->pBuffer;
	}
}

void NX_CRGBDraw::FreeBuffer()
{
	for(int32_t i = 0 ; i <m_DspInfo.m_iNumBuffer ; i++)
	{
		if(pRGBData[i] != NULL)
		{
			NX_FreeMemory(pRGBData[i]);
		}
	}
}

//------------------------------------------------------------------------------
int32_t NX_CRGBDraw::Init(NX_RGB_DRAW_INFO *pInfo)
{
	int32_t hDrmFd = -1;

	memset(&m_DspInfo, 0, sizeof(NX_RGB_DRAW_INFO));

	if(pInfo->drmFd < 0)
		m_DspInfo.drmFd = drmOpen( "nexell", NULL );
	else
	{
		m_DspInfo.drmFd = pInfo->drmFd;
	}

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

	m_DspInfo.planeId 	= pInfo->planeId;
	m_DspInfo.crtcId 	= pInfo->crtcId;
	m_DspInfo.m_iDspX	= pInfo->m_iDspX;
	m_DspInfo.m_iDspY	= pInfo->m_iDspY;
	m_DspInfo.m_iDspWidth = pInfo->m_iDspWidth;
	m_DspInfo.m_iDspHeight = pInfo->m_iDspHeight;
	m_DspInfo.m_iNumBuffer = pInfo->m_iNumBuffer;
	m_DspInfo.uDrmFormat = pInfo->uDrmFormat;

	return 0;
}

//------------------------------------------------------------------------------
void NX_CRGBDraw::Deinit( void )
{
	if( m_DspInfo.drmFd > 0 )
	{
		// clean up object here
		for( int32_t i = 0; i < MAX_MEMORY_NUM; i++ )
		{
			if( 0 < m_BufferId[i] )
			{
				drmModeRmFB( m_DspInfo.drmFd, m_BufferId[i] );
				m_BufferId[i] = 0;
			}
		}

		if( 0 <= m_DspInfo.drmFd )
		{
			//drmClose( m_DspInfo.drmFd );
			m_DspInfo.drmFd = -1;
		}
	}


}

//------------------------------------------------------------------------------
int32_t NX_CRGBDraw::DrmIoctl( int32_t fd, unsigned long request, void *pArg )
{
	int32_t ret;

	do {
		ret = ioctl(fd, request, pArg);
	} while (ret == -1 && (errno == EINTR || errno == EAGAIN));

	return ret;
}

//------------------------------------------------------------------------------
int32_t NX_CRGBDraw::ImportGemFromFlink( int32_t fd, uint32_t flinkName )
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
int32_t NX_CRGBDraw::Render(int32_t m_iBufferIdx)
{
	int32_t err;

	if(m_BufferId[m_iBufferIdx] == 0)
	{
		uint32_t handles[4] = { 0, };
		uint32_t pitches[4] = { 0, };
		uint32_t offsets[4] = { 0, };

		pitches[0] = m_DspInfo.m_iDspWidth * 4;
		pitches[1] = 0;
		pitches[2] = 0;
		pitches[3] = 0;

		offsets[0] = 0;
		offsets[1] = 0;
		offsets[2] = 0;
		offsets[3] = 0;

		handles[0] = handles[1] = handles[2] = handles[3] = ImportGemFromFlink( m_DspInfo.drmFd, pRGBData[m_iBufferIdx]->flink);
		if (drmModeAddFB2(m_DspInfo.drmFd, m_DspInfo.m_iDspWidth, m_DspInfo.m_iDspHeight, m_DspInfo.uDrmFormat,
			handles, pitches, offsets, &m_BufferId[m_iBufferIdx], 0)) {
				fprintf(stderr, "failed to add fb: %s\n", strerror(errno));
		}
	}

	err = drmModeSetPlane( m_DspInfo.drmFd, m_DspInfo.planeId, m_DspInfo.crtcId, m_BufferId[m_iBufferIdx], 0,
				m_DspInfo.m_iDspX, m_DspInfo.m_iDspY, m_DspInfo.m_iDspWidth, m_DspInfo.m_iDspHeight,
				m_DspInfo.m_iDspX << 16, m_DspInfo.m_iDspY << 16, m_DspInfo.m_iDspWidth << 16, m_DspInfo.m_iDspHeight << 16 );

	return err;

}

//------------------------------------------------------------------------------
void NX_CRGBDraw::GetMemInfo(NX_RGB_DRAW_MEMORY_INFO *m_rgbMemInfo, int32_t m_iBufferIdx)
{
		m_rgbMemInfo->mem_size = m_MemInfo[m_iBufferIdx].mem_size;
		m_rgbMemInfo->pBuffer = m_MemInfo[m_iBufferIdx].pBuffer;
}


//------------------------------------------------------------------------------
void NX_CRGBDraw::FillColorBackGround(int32_t dsp_width, int32_t width, int32_t height, int32_t start_x, int32_t start_y, uint32_t color)
{
	for(int32_t i=0; i<m_DspInfo.m_iNumBuffer; i++)
	{
		uint32_t *pbuf = (uint32_t*)m_MemInfo[i].pBuffer;
		pbuf += dsp_width * start_y + start_x;

		for(int32_t j=0; j<height; j++)
		{
			for(int32_t k=0; k<width; k++)
			{
				pbuf[k] = color;
			}
			pbuf += dsp_width;
		}
	}

}