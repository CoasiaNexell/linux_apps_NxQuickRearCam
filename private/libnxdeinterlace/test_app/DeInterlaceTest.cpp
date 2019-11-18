#include <unistd.h>     // getopt & optarg
#include <stdio.h>      // printf
#include <string.h>     // strdup
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>   // gettimeofday
#include "../include/nx_deinterlace.h"
#include <drm_fourcc.h>
#include <nx_video_alloc.h>

#define	INPUT_BUFFER_NUM                 4
#define DEINTERLACE_BUFFER_NUM           4

#define ROUND_UP_16(num) (((num)+15)&~15)
#define ROUND_UP_32(num) (((num)+31)&~31)


static uint64_t NX_GetTickCount( void )
{
	uint64_t ret;
	struct timeval	tv;
	struct timezone	zv;
	gettimeofday( &tv, &zv );
	ret = ((uint64_t)tv.tv_sec)*1000000 + tv.tv_usec;
	return ret;
}



int32_t main( int32_t argc, char *argv[] )
{
	char *chIn, *chOut = NULL;
	int32_t iMode, iWidth, iHeight, opt;
	int32_t iNumPlane = 1;
	int32_t strideWidth[3] = {0,};
	int32_t strideHeight[3] = {0,};

	while( -1 != (opt=getopt(argc, argv, "i:o:s:m:h")))
	{
		switch( opt ) {
			case 'i' : chIn = strdup( optarg );	break;
			case 'o' : chOut = strdup( optarg );	break;
			case 's' : sscanf( optarg, "%d,%d", &iWidth, &iHeight );	break;
			case 'm' : iMode = atoi( optarg ); break;
			case 'h' : printf("-i [input file name], -o [output_file_name], -s [width],[height] -m[mode] (0:none, 1:discard, 2:mean, 3:blend, 4:bob, 5:linear)\n");		return 0;
			default : break;
		}
	}

	{
		void *hDeInterlace = NULL;
		NX_VID_MEMORY_HANDLE hInMem[INPUT_BUFFER_NUM];
		NX_VID_MEMORY_HANDLE hOutMem[DEINTERLACE_BUFFER_NUM];
		int32_t iFrmCnt = 0;
		uint64_t startTime, endTime, totalTime = 0;
		uint8_t *pIndata;

		FILE *fpIn = fopen(chIn, "rb");
		FILE *fpOut = ( chOut )  ? ( fopen(chOut, "wb") ) : ( NULL );

		if ( (hDeInterlace = NX_DeInterlaceOpen( (DEINTERLACE_MODE)iMode )) == NULL )
		{
			printf("NX_DeInterlaceOpen() is failed!!!\n");
			return -1;
		}
		printf("NX_DeInterlaceOpen() is Success!!!\n");

		for( int32_t i=0 ; i<INPUT_BUFFER_NUM ; i++)
		{
			//hInMem[i] = NX_VideoAllocateMemory( 4096, iWidth, iHeight, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );
			hInMem[i] = NX_AllocateVideoMemory(-1,  iWidth, iHeight, iNumPlane, DRM_FORMAT_YUV420, 4096 );
			if ( 0 == hInMem[i] ) {
				printf("NX_VideoAllocMemory(64, %d, %d,...) failed. (i=%d)\n", iWidth, iHeight, i);
				return -1;

			}

			NX_MapVideoMemory(hInMem[i]);
		}

		for( int32_t i=0 ; i<DEINTERLACE_BUFFER_NUM ; i++ )
		{
			//hOutMem[i] = NX_VideoAllocateMemory( 4096, iWidth, iHeight, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );
			hOutMem[i] = NX_AllocateVideoMemory(-1, iWidth, iHeight, iNumPlane, DRM_FORMAT_YUV420, 4096 );
			if ( 0 == hOutMem[i] ) {
				printf("NX_VideoAllocMemory(64, %d, %d,...) failed. (i=%d)\n", iWidth, iHeight, i);
				return -1;

			}
			NX_MapVideoMemory(hOutMem[i]);
		}

		strideWidth[0] = ROUND_UP_32(hInMem[0]->stride[0]);
		strideWidth[1] = ROUND_UP_16(strideWidth[0]>>1);
		strideWidth[2] = strideWidth[1];

		strideHeight[0] = ROUND_UP_16(hInMem[0]->height);
		strideHeight[1] = ROUND_UP_16(hInMem[0]->height >> 1);
		strideHeight[2] = strideHeight[1];
		
		while ( 1 )
		{
			int32_t iInputIdx = iFrmCnt%INPUT_BUFFER_NUM;
			pIndata = (uint8_t*) hInMem[iInputIdx]->pBuffer;
			for(int32_t h = 0; h < strideHeight[0]; h++)
			{
				fread((uint8_t *)pIndata, 1, iWidth * iHeight, fpIn);
				pIndata += strideWidth[0];
			}
				
			pIndata = (uint8_t*)( pIndata + (strideWidth[0] * strideHeight[0]));
			for(int32_t h = 0; h < strideHeight[1]; h++)
			{
				fread((uint8_t *)pIndata, 1, iWidth * iHeight / 4, fpIn);		
				pIndata += strideWidth[1];
			}

			pIndata = (uint8_t*)( pIndata + (strideWidth[0] * strideHeight[0]) + (strideWidth[1] * strideHeight[1]));	
			for(int32_t h = 0; h < strideHeight[2] ; h++)
			{
				fread((uint8_t *)pIndata , 1, iWidth * iHeight / 4, fpIn);
				pIndata += strideWidth[2];
			}
			

			startTime = NX_GetTickCount();
			if ( NX_DeInterlaceFrame( hDeInterlace, hInMem[iInputIdx], 0, hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM] ) < 0 )
			{
				printf("NX_DeInterlaceFrame() failed!!!\n");
				break;
			}
			endTime = NX_GetTickCount();
			totalTime += (endTime - startTime);
			printf("%d Frame : Deinterlace Timer = %lld\n", iFrmCnt, endTime - startTime);

			if ( fpOut )
			{
				int32_t h;
				uint8_t *pbyImg = (uint8_t *)(hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->pBuffer);
				for(h=0 ; h<hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->height ; h++)
				{
					fwrite(pbyImg, 1, hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->width, fpOut);
					pbyImg += strideWidth[0];
				}

				pbyImg = (uint8_t *)(hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->pBuffer + (strideWidth[0] * strideHeight[0]));
				for(h=0 ; h<hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->height/2 ; h++)
				{
					fwrite(pbyImg, 1, hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->width/2, fpOut);
					pbyImg += strideWidth[1];
				}

				pbyImg = (uint8_t *)(hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->pBuffer + (strideWidth[0] * strideHeight[0])+(strideWidth[1]*strideHeight[1]));
				for(h=0 ; h<hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->height/2 ; h++)
				{
					fwrite(pbyImg, 1, hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->width/2, fpOut);
					pbyImg += strideWidth[2];
				}
			}

			iFrmCnt++;
		}

		printf("average timer : %lld ms \n", totalTime/iFrmCnt );

		for (int32_t i=0 ; i<INPUT_BUFFER_NUM ; i++)
			NX_FreeVideoMemory( hInMem[i] );

		for (int32_t i=0 ; i<DEINTERLACE_BUFFER_NUM ; i++)
			NX_FreeVideoMemory( hOutMem[i] );

		if ( hDeInterlace )
			NX_DeInterlaceClose( hDeInterlace );

		fclose(fpIn);
		fclose(fpOut);
	}

	return 0;
}
