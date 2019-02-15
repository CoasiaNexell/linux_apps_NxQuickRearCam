//
//  Nexel De-Interlacer API
//


#ifndef __NX_DEINTERLACER_H__
#define __NX_DEINTERLACER_H__

//#include <nxp_video_alloc.h>
//#include <nx_video_alloc.h>


typedef enum {
	NON_DEINTERLACER		= 0,
	DEINTERLACE_DISCARD,
	DEINTERLACE_MEAN,
	DEINTERLACE_BLEND,
	DEINTERLACE_BOB,
	DEINTERLACE_LINEAR,
	//DEINTERLACE_X,
	//DEINTERLACE_YADIF,
	//DEINTERLACE_YADIF2X,
	//DEINTERLACE_PHOSPHOR,
	//DEINTERLACE_IVTC,
	UNKNOWN_TYPE			= 0xFF
} DEINTERLACE_MODE;

typedef struct
{
	int32_t                    iMajor;
	int32_t                    iMinor;
	int32_t                    iPatch;
} NX_DEINTERLACER_VERSION;



typedef struct
{
	int32_t width;
	int32_t height;

	int32_t stride[3];

	uint8_t *pY;
	uint8_t *pU;
	uint8_t *pV;

} NX_DEINTERLACER_INFO;
//


#ifdef __cplusplus
extern "C" {
#endif


//  De-Interlacer API Functions
//
int32_t NX_DeInterlaceGetVersion( NX_DEINTERLACER_VERSION *pstVersion );
void *NX_DeInterlaceOpen( DEINTERLACE_MODE eMode );
int32_t NX_DeInterlaceClose( void * );
int32_t NX_DeInterlaceFrame( void *pvHandle, NX_DEINTERLACER_INFO *hInImage, int32_t iTopFieldFirst, NX_DEINTERLACER_INFO *hOutImage);


#ifdef __cplusplus
}
#endif

#endif  //  __NX_DEINTERLACER_H__
