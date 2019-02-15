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

#ifndef __NX_DRAWPARKINGGUIDELINE_H__
#define __NX_DRAWPARKINGGUIDELINE_H__

#ifdef __cplusplus

//#include "NX_CBaseFilter.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <nxp_video_alloc.h>

#define MAX_MEMORY_NUM 8

typedef struct NX_RGB_DRAW_INFO {
	uint32_t 	planeId;
	uint32_t 	crtcId;

	int32_t		m_iDspX;
	int32_t		m_iDspY;
	int32_t 	m_iDspWidth;
	int32_t		m_iDspHeight;

	int32_t		m_iNumBuffer;

	int32_t		uDrmFormat;

	int32_t 	drmFd;

}NX_RGB_DRAW_INFO;

typedef struct NX_RGB_DRAW_MEMORY_INFO {
	int32_t mem_size;
	int32_t bufferIdx;
	void *pBuffer;
}NX_RGB_DRAW_MEMORY_INFO;


class NX_CRGBDraw
{
public:
	NX_CRGBDraw();
	virtual ~NX_CRGBDraw();

public:
	int32_t Init(NX_RGB_DRAW_INFO *pInfo);
	void Deinit(void);
	int32_t Render(int32_t);
	void GetMemInfo(NX_RGB_DRAW_MEMORY_INFO *m_rgbMemInfo, int32_t m_iBufferIdx);
	void FillColorBackGround(int32_t dsp_width, int32_t width, int32_t height, int32_t start_x, int32_t start_y, uint32_t color);
private:
	int32_t DrmIoctl( int32_t fd, unsigned long request, void *pArg );
	int32_t ImportGemFromFlink( int32_t fd, uint32_t flinkName );

private:
	NX_RGB_DRAW_INFO m_DspInfo;
	NX_RGB_DRAW_MEMORY_INFO m_MemInfo[MAX_MEMORY_NUM];
	NX_MEMORY_HANDLE pRGBData[MAX_MEMORY_NUM];

	int32_t curBufIdx;

	uint32_t		m_BufferId[MAX_MEMORY_NUM];

private:
	NX_CRGBDraw (const NX_CRGBDraw &Ref);
	NX_CRGBDraw &operator=(const NX_CRGBDraw &Ref);
};


#endif
#endif //__NX_DRAWPARKINGGUIDELINE_H__

