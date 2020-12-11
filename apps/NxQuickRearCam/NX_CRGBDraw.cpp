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
    drmModeRes *res;
	drmModePlaneRes *plane_res;
	struct drm_mode_create_dumb create;
    struct drm_mode_map_dumb map;

	mem_size = pInfo->m_iDspWidth * pInfo->m_iDspHeight * 4;

	memset(&m_DspInfo, 0, sizeof(NX_RGB_DRAW_INFO));

	if(pInfo->drmFd < 0)
		m_DspInfo.drmFd = drmOpen( "nexell", NULL );
	else
	{
		m_DspInfo.drmFd = pInfo->drmFd;
	}

	for(int i=0; i<pInfo->m_iNumBuffer; i++)
	{
		// Get crtc_id & conn_id
		res = drmModeGetResources(m_DspInfo.drmFd);
		m_MemInfo[i].crtc_id = res->crtcs[i];
		m_MemInfo[i].conn_id = res->connectors[i];

		// Get conn pointer
		m_MemInfo[i].conn = drmModeGetConnector(m_DspInfo.drmFd, m_MemInfo[i].conn_id);
		m_MemInfo[i].width = m_MemInfo[i].conn->modes[i].hdisplay;
		m_MemInfo[i].height = m_MemInfo[i].conn->modes[i].vdisplay;

		/* create a dumb-buffer, the pixel format is XRGB888 */
		create.width = m_MemInfo[i].width;
		create.height = m_MemInfo[i].height;
		create.bpp = 32;
		drmIoctl(m_DspInfo.drmFd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
		m_MemInfo[i].pitch = create.pitch;
		m_MemInfo[i].size = create.size;
		m_MemInfo[i].handle = create.handle;

		/* map the dumb-buffer to userspace */
		map.handle = create.handle;
		drmIoctl(m_DspInfo.drmFd, DRM_IOCTL_MODE_MAP_DUMB, &map);
		m_MemInfo[i].pBuffer = mmap(0, create.size, PROT_READ | PROT_WRITE, \
				MAP_SHARED, m_DspInfo.drmFd, map.offset);
		m_MemInfo[i].mem_size = ALIGN(mem_size, pInfo->m_iDspWidth * 4);
	}

	printf("=== <Alloc> drmModeAddFB : fd(%d) ===\n", m_DspInfo.drmFd);
	drmModeAddFB(m_DspInfo.drmFd, m_MemInfo[0].width, m_MemInfo[0].height, 24, 32, m_MemInfo[0].pitch, \
			m_MemInfo[0].handle, &m_MemInfo[0].fb_id);

	printf("=== <Alloc> drmModeSetCrtc : crtcId(%d), fb_id(%d) ===\n", m_MemInfo[0].crtc_id, m_MemInfo[0].fb_id);
	drmModeSetCrtc(m_DspInfo.drmFd, m_MemInfo[0].crtc_id, m_MemInfo[0].fb_id, 0, 0, &m_MemInfo[0].conn_id, 1, &m_MemInfo[0].conn->modes[0]);

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
		printf("=== drmModeRmFB fb_id(%d) ===\n", m_MemInfo[0].fb_id);
		drmModeRmFB( m_DspInfo.drmFd, m_MemInfo[0].fb_id);
		m_MemInfo[0].fb_id = 0;

		if( 0 <= m_DspInfo.drmFd )
		{
			//drmClose( m_DspInfo.drmFd );
			m_DspInfo.drmFd = -1;
		}

		printf("=== m_DspInfo.drmFd(%d) ===\n", m_DspInfo.drmFd);
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

	printf("=== <Render> drmModeAddFB m_DspInfo.drmFd(%d) ===\n", m_DspInfo.drmFd);
	drmModeAddFB(m_DspInfo.drmFd, m_MemInfo[0].width, m_MemInfo[0].height, 24, 32, m_MemInfo[0].pitch, \
			m_MemInfo[0].handle, &m_MemInfo[0].fb_id);

	printf("=== <Render> planeId(%d), crtcId(%d), fb_id(%d) ===\n", m_DspInfo.planeId, m_DspInfo.crtcId, m_MemInfo[0].fb_id);
    err = drmModeSetPlane( m_DspInfo.drmFd, m_DspInfo.planeId, m_DspInfo.crtcId, m_MemInfo[0].fb_id, 0,
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