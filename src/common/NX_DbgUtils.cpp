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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
//#include <nx_fourcc.h>

#include "NX_DbgUtils.h"

// #define NX_DBG_OFF

#define NX_DTAG		"[NX_DbgUtils]"
#include <NX_DbgMsg.h>

//------------------------------------------------------------------------------
void NxDumpData( void *pData, int32_t nSize )
{
	int32_t i=0, offset = 0;
	char tmp[32];
	char lineBuf[1024];

	uint8_t *data = (uint8_t*)pData;

	while( offset < nSize )
	{
		sprintf( lineBuf, "%08lx : ", (unsigned long)offset );
		for( i=0 ; i<16 ; ++i )
		{
			if( i == 8 ) {
				strcat( lineBuf, " " );
			}

			if( offset+i >= nSize ) {
				strcat( lineBuf, " " );
			}
			else {
				sprintf( tmp, "%02x ", data[offset+i] );
				strcat( lineBuf, tmp );
			}
		}

		strcat( lineBuf, " " );

		// Add ACSII A~Z, & Number & String
		for( i=0 ; i<16 ; ++i )
		{
			if( offset+i >= nSize ) {
				break;
			}
			else {
				if( isprint( data[offset+i] ) ) {
					sprintf( tmp, "%c", data[offset+i] );
					strcat( lineBuf, tmp );
				}
				else {
					strcat( lineBuf, "." );
				}
			}
		}

		strcat( lineBuf, "\n" );
		NxDbgMsg( NX_DBG_INFO, "%s", lineBuf );
		offset += 16;
	}
}

#if 0 // TBD
//------------------------------------------------------------------------------
int32_t NxClearYuvBuf( NX_VID_MEMORY_HANDLE hInMem, uint8_t iValue )
{
	int32_t h;
	uint8_t *pbyImg = (uint8_t *)(hInMem->luVirAddr);
	int32_t width = hInMem->imgWidth;
	int32_t height = hInMem->imgHeight;
	int32_t cWidth, cHeight;

	cWidth  = width;
	cHeight = height >> 1;


	for(h=0 ; h<height ; h++)
	{
		memset(pbyImg, iValue, width);
		pbyImg += hInMem->luStride;
	}

	pbyImg = (uint8_t *)(hInMem->cbVirAddr);
	for(h=0 ; h<cHeight ; h++)
	{
		memset(pbyImg, iValue, cWidth);
		pbyImg += hInMem->luStride;
	}

	pbyImg = (uint8_t *)(hInMem->crVirAddr);
	for(h=0 ; h<cHeight ; h++)
	{
		memset(pbyImg, iValue, cWidth);
		pbyImg += hInMem->crStride;
	}

	return 0;
}

//------------------------------------------------------------------------------
int32_t NxDumpYuv2File( NX_VID_MEMORY_HANDLE hInMem, const char *pOutFile )
{
	FILE *fp = NULL;

	if( NULL != (fp = fopen( pOutFile, "w" ) ) )
	{
		int32_t h;
		uint8_t *pbyImg = (uint8_t *)(hInMem->luVirAddr);
		int32_t width	= hInMem->imgWidth;
		int32_t height	= hInMem->imgHeight;
		int32_t cWidth, cHeight;

		switch( hInMem->fourCC )
		{
			case FOURCC_MVS0:
				cWidth  = width >> 1;
				cHeight = height >> 1;
				break;
			case FOURCC_MVS2:
			case FOURCC_H422:
				cWidth  = width >> 1;
				cHeight = height;
				break;
			case FOURCC_V422:
				cWidth  = width;
				cHeight = height >> 1;
				break;
			case FOURCC_MVS4:
				cWidth  = width;
				cHeight = height;
				break;
			case FOURCC_GRAY:
				cWidth  = 0;
				cHeight = 0;
				break;
			default:
				NxDbgMsg( NX_DBG_ERR, "Unknown fourCC type.\n" );
				return -1;
		}

		for(h=0 ; h<height ; h++)
		{
			fwrite(pbyImg, 1, width, fp);
			pbyImg += hInMem->luStride;
		}

		pbyImg = (uint8_t *)(hInMem->cbVirAddr);
		for(h=0 ; h<cHeight ; h++)
		{
			fwrite(pbyImg, 1, cWidth, fp);
			pbyImg += hInMem->cbStride;
		}

		pbyImg = (uint8_t *)(hInMem->crVirAddr);
		for(h=0 ; h<cHeight ; h++)
		{
			fwrite(pbyImg, 1, cWidth, fp);
			pbyImg += hInMem->crStride;
		}

		fclose( fp );

		NxDbgMsg( NX_DBG_INFO, "NxDumpYuv2File() Done. ( %s )\n", pOutFile );
		return 0;
	}

	return -1;
}
#endif