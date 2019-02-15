#ifndef __NX_BACKGEARDETECT_H__
#define __NX_BACKGEARDETECT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NX_BACKGEAR_NOTDETECTED = 0x0,
	NX_BACKGEAR_DETECTED = 0x1,
};

int32_t NX_StartBackGearDetectService( int32_t nGpio, int32_t nChkDelay );
void NX_StopBackGearDetectService();
void NX_RegisterBackGearEventCallBack(void *, void (*callback)(void *, int32_t));

#ifdef __cplusplus
}
#endif

#endif  //__NX_BACKGEARDETECT_H__