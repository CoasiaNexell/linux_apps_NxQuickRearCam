/*****************************************************************************
 * Copyright (C) 2000-2011 VLC authors and VideoLAN
 * $Id: 2b1d9e3c812d86d4af91ef01c0bb9beba05c358f $
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#include "deinterlace_def.h"
#include "merge.h"
#include "algo_basic.h"
#include "algo_x.h"
#include "./include/nx_deinterlace.h"
#include <stdio.h>


int32_t NX_DeInterlaceGetVersion( NX_DEINTERLACER_VERSION *pstVersion )
{
	pstVersion->iMajor = NX_DEINTERLACE_VER_MAJOR;
	pstVersion->iMinor = NX_DEINTERLACE_VER_MINOR;
	pstVersion->iPatch = NX_DEINTERLACE_VER_PATCH;
	return 0;
}

void *NX_DeInterlaceOpen( DEINTERLACE_MODE eMode )
{
	NX_VIDEO_DEINTERLACE_INFO *pstHandle = (NX_VIDEO_DEINTERLACE_INFO *)malloc( sizeof(NX_VIDEO_DEINTERLACE_INFO) );

	memset( pstHandle, 0, sizeof(NX_VIDEO_DEINTERLACE_INFO) );

	pstHandle->iMode = eMode;
	if ( (pstHandle->iMode < NON_DEINTERLACER) || (pstHandle->iMode > DEINTERLACE_LINEAR) )
	{
		return (void *)NULL;
	}

	if ( pstHandle->iMode == NON_DEINTERLACER )
	{
		return (void *)pstHandle;
	}
	pstHandle->p_filter = (filter_t *)malloc(sizeof(filter_t));
	pstHandle->p_filter->p_sys = (filter_sys_t *)malloc(sizeof(filter_sys_t));

	unsigned pixel_size = 1;            // 1 Byte = 8 Bit
#if defined(CAN_COMPILE_C_ALTIVEC)
	if( pixel_size == 1 && vlc_CPU_ALTIVEC() )
		pstHandle->p_filter->p_sys->pf_merge = MergeAltivec;
	else
#endif
#if defined(CAN_COMPILE_SSE2)
	if( vlc_CPU_SSE2() )
	{
		pstHandle->p_filter->p_sys->pf_merge = pixel_size == 1 ? Merge8BitSSE2 : Merge16BitSSE2;
		pstHandle->p_filter->p_sys->pf_end_merge = EndMMX;
	}
	else
#endif
#if defined(CAN_COMPILE_MMXEXT)
	if( pixel_size == 1 && vlc_CPU_MMXEXT() )
	{
		pstHandle->p_filter->p_sys->pf_merge = MergeMMXEXT;
		pstHandle->p_filter->p_sys->pf_end_merge = EndMMX;
	}
	else
#endif
#if defined(CAN_COMPILE_3DNOW)
	if( pixel_size == 1 && vlc_CPU_3dNOW() )
	{
		pstHandle->p_filter->p_sys->pf_merge = Merge3DNow;
		pstHandle->p_filter->p_sys->pf_end_merge = End3DNow;
	}
	else
#endif
#if defined(CAN_COMPILE_ARM)
	//if( vlc_CPU_ARM_NEON() )
	if ( 1 )
		pstHandle->p_filter->p_sys->pf_merge = pixel_size == 1 ? merge8_arm_neon : merge16_arm_neon;
	//else
	//if( vlc_CPU_ARMv6() )
	//    pstHandle->p_filter->p_sys->pf_merge = pixel_size == 1 ? merge8_armv6 : merge16_armv6;
	else
#endif
	{
		pstHandle->p_filter->p_sys->pf_merge = pixel_size == 1 ? Merge8BitGeneric : Merge16BitGeneric;
#if defined(__i386__) || defined(__x86_64__)
		pstHandle->p_filter->p_sys->pf_end_merge = NULL;
#endif
	}

	return (void *)pstHandle;
}

int32_t NX_DeInterlaceClose( void *pvHandle )
{
	NX_VIDEO_DEINTERLACE_INFO *pstHandle = (NX_VIDEO_DEINTERLACE_INFO *)pvHandle;

	if( !pstHandle )
	{
		printf("invalid deinterlace handle!!!\n");
		return -1;
	}

	if ( pstHandle->p_filter )
	{
		if ( pstHandle->p_filter->p_sys )
			free( pstHandle->p_filter->p_sys );
		free( pstHandle->p_filter );
	}

	free( pstHandle );

	return 0;
}


#define ROUND_UP_16(num) (((num)+15)&~15)
#define ROUND_UP_32(num) (((num)+31)&~31)

int32_t NX_DeInterlaceFrame( void *pvHandle, NX_DEINTERLACER_INFO *hInImage, int32_t iTopFieldFirst, NX_DEINTERLACER_INFO *hOutImage)
{
	NX_VIDEO_DEINTERLACE_INFO *pstHandle = (NX_VIDEO_DEINTERLACE_INFO *)pvHandle;
	int32_t in_strideWidth[3] = {0,};
	int32_t in_strideHeight[3] = {0,};
	int32_t out_strideWidth[3] = {0,};
	int32_t out_strideHeight[3] = {0,};

	in_strideWidth[0] = hInImage->stride[0];
	in_strideWidth[1] = hInImage->stride[1];
	in_strideWidth[2] = hInImage->stride[2];

	out_strideWidth[0] = hOutImage->stride[0];
	out_strideWidth[1] = hOutImage->stride[1];
	out_strideWidth[2] = hOutImage->stride[2];


	in_strideHeight[0] = ROUND_UP_16(hInImage->height);
	in_strideHeight[1] = ROUND_UP_16(hInImage->height >> 1);
	in_strideHeight[2] = in_strideHeight[1];

	out_strideHeight[0] = ROUND_UP_16(hOutImage->height);
	out_strideHeight[1] = ROUND_UP_16(hOutImage->height >> 1);
	out_strideHeight[2] = out_strideHeight[1];


	if ( !pstHandle )
	{
		printf("invalid deinterlace handle!!!\n");
		return -1;
	}

	picture_t stPicSrc;
	stPicSrc.i_planes = 3;


	stPicSrc.p[0].p_pixels = (uint8_t *)hInImage->pY;
	stPicSrc.p[0].i_pitch = in_strideWidth[0];
	stPicSrc.p[1].p_pixels = (uint8_t *)hInImage->pU;
	stPicSrc.p[1].i_pitch = in_strideWidth[1];
	stPicSrc.p[2].p_pixels = (uint8_t *)hInImage->pV;
	stPicSrc.p[2].i_pitch = in_strideWidth[2];

	picture_t stPicDst;
	stPicDst.p[0].p_pixels = (uint8_t *)hOutImage->pY;
	stPicDst.p[0].i_pitch = out_strideWidth[0];
	stPicDst.p[0].i_visible_lines = hOutImage->height;
	stPicDst.p[0].i_visible_pitch = hOutImage->width * hOutImage->height;
	stPicDst.p[1].p_pixels = (uint8_t *)hOutImage->pU;
	stPicDst.p[1].i_pitch = out_strideWidth[1];
	stPicDst.p[1].i_visible_lines = hOutImage->height >> 1;
	stPicDst.p[1].i_visible_pitch = hOutImage->width * hOutImage->height >> 2;
	stPicDst.p[2].p_pixels = (uint8_t *)hOutImage->pV;
	stPicDst.p[2].i_pitch = out_strideWidth[2];
	stPicDst.p[2].i_visible_lines = hOutImage->height >> 1;
	stPicDst.p[2].i_visible_pitch = hOutImage->width * hOutImage->height >> 2;


	switch( pstHandle->iMode )
	{
		case DEINTERLACE_DISCARD:
			RenderDiscard( &stPicDst, &stPicSrc, 0 );
			break;

		case DEINTERLACE_BOB:
			RenderBob( &stPicDst, &stPicSrc, !iTopFieldFirst );
			/*if( p_dst[1] )
				RenderBob( p_dst[1], p_pic, iTopFieldFirst );
			if( p_dst[2] )
				RenderBob( p_dst[2], p_pic, !iTopFieldFirst );*/
			break;

		case DEINTERLACE_LINEAR:
			RenderLinear( pstHandle->p_filter, &stPicDst, &stPicSrc, !iTopFieldFirst );
			/*( p_dst[1] )
				RenderLinear( p_filter, p_dst[1], p_pic, iTopFieldFirst );
			if( p_dst[2] )
				RenderLinear( p_filter, p_dst[2], p_pic, !iTopFieldFirst );*/
			break;

		case DEINTERLACE_MEAN:
			RenderMean( pstHandle->p_filter, &stPicDst, &stPicSrc );
			break;

		case DEINTERLACE_BLEND:
			RenderBlend( pstHandle->p_filter, &stPicDst, &stPicSrc );
			break;

#if 0
		case DEINTERLACE_X:
			RenderX( &stPicDst, &stPicSrc );
			break;

		case DEINTERLACE_YADIF:
			RenderYadif( pstHandle->p_filter, &stPicDst, &stPicSrc, 0, 0);
			break;

		case DEINTERLACE_YADIF2X:
			RenderYadif( pstHandle->p_filter, &stPicDst, &stPicSrc, 0, !iTopFieldFirst );
			/*if( p_dst[1] )
				RenderYadif( p_filter, p_dst[1], p_pic, 1, iTopFieldFirst );
			if( p_dst[2] )
				RenderYadif( p_filter, p_dst[2], p_pic, 2, !iTopFieldFirst );*/
			break;

		case DEINTERLACE_PHOSPHOR:
			RenderPhosphor( pstHandle->p_filter, &stPicDst, 0, !iTopFieldFirst );
			/*if( p_dst[1] )
				RenderPhosphor( p_filter, p_dst[1], 1, iTopFieldFirst );
			if( p_dst[2] )
				RenderPhosphor( p_filter, p_dst[2], 2, !iTopFieldFirst );*/
			break;

		case DEINTERLACE_IVTC:
			/* Note: RenderIVTC will automatically drop the duplicate frames produced by IVTC. This is part of normal operation. */
			RenderIVTC( pstHandle->p_filter, &stPicDst);
			break;
#endif
		default:
			return -1;
	}

	return 0;
}
