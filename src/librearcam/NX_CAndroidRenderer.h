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

#ifndef __NX_CANDROIDRENDERER_H__
#define __NX_CANDROIDRENDERER_H__

#ifdef __cplusplus

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>

#include <nx_video_alloc.h>

using namespace android;

class NX_CAndroidRenderer
{
public:
	NX_CAndroidRenderer( ANativeWindow *pYuvWindow );
	NX_CAndroidRenderer( int32_t iWidth, int32_t iHeight );
	~NX_CAndroidRenderer();

public:
	int32_t GetBuffers( int32_t iNumBuf, int32_t iWidth, int32_t iHeight, NX_VID_MEMORY_HANDLE **pMemHandle );

	int32_t QueueBuffer( int32_t iIndex );
	int32_t DequeueBuffer( void );

	int32_t SetPosition( int32_t iX, int32_t iY, int32_t iWidth, int32_t iHeight );

private:
	enum { MAX_BUFFER_NUM = 32 };

	sp<SurfaceControl> 		m_SurfaceControl;	//	YUV Surface Controller
	ANativeWindow*			m_pYuvWindow;
	ANativeWindowBuffer*	m_pYuvANBuffer[MAX_BUFFER_NUM];
	NX_VID_MEMORY_HANDLE	m_hMemoryHandle[MAX_BUFFER_NUM];

	int32_t					m_iNumBuffer;

private:
	NX_CAndroidRenderer(NX_CAndroidRenderer &Ref);
	NX_CAndroidRenderer &operator=(NX_CAndroidRenderer &Ref);
};

#endif	// __cplusplus
#endif	// __NX_CANDROIDRENDERER_H__
