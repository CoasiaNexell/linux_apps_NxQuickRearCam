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

#ifndef __NX_DBGUTILS_H__
#define __NX_DBGUTILS_H__

#include <stdint.h>
//#include <nx_alloc_mem.h>
#include <nx_video_alloc.h>

void	NxDumpData( void *pData, int nSize );

int32_t NxClearYuvBuf( NX_VID_MEMORY_HANDLE hInMem, uint8_t iValue );
int32_t NxDumpYuv2File( NX_VID_MEMORY_HANDLE hInMem, const char *pOutFile );

#endif	// __NX_DBGUTILS_H__
