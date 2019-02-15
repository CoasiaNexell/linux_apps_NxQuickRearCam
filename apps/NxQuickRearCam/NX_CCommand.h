#ifndef __NX_COMMANDSERVER_H__
#define __NX_COMMANDSERVER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum{
	STOP = 1,
	RUN,
	MAX_COMMAND
};

void* NX_GetCommandHandle();
int32_t NX_StartCommandService(void*, char*);
void NX_StopCommandService(void *, char*);
void NX_RegisterCommandEventCallBack(void*, void (*callback)(int32_t));
#ifdef __cplusplus
}
#endif

#endif  //__NX_BACKGEARDETECT_H__