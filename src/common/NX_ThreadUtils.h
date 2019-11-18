#ifndef __NX_THREADUTILS_H__
#define __NX_THREADUTILS_H__

#include <stdint.h>
#include <pthread.h>



int32_t NX_AdjustThreadPriority( pthread_attr_t *thread_attrs, int32_t policy, int32_t priority );

int32_t NX_ThreadAttributeDestroy( pthread_attr_t *thread_attrs );

#endif  //__NX_THREADUTILS_H__