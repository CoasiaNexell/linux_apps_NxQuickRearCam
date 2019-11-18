//------------------------------------------------------------------------------
//
//	Copyright (C) 2019 Nexell Co. All Rights Reserved
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
#ifndef __NX_THREADUTILS__
#define __NX_THREADUTILS__

#include "NX_ThreadUtils.h"

#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_ThreadUtils]"
#include <NX_DbgMsg.h>


int32_t NX_AdjustThreadPriority(pthread_attr_t *thread_attrs,int32_t policy, int32_t priority )
{
	int32_t ret = 0;
	struct sched_param param;
	struct sched_param param_res;

	ret = pthread_attr_init(thread_attrs);
	if(ret < 0)
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, pthread_attr_init\n" );
		return -1;
	}

	ret = pthread_attr_setschedpolicy(thread_attrs, policy);
	if(ret < 0)
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, pthread_attr_setschedpolicy\n" );
		return -1;
	}

	param.sched_priority = priority;

	ret = pthread_attr_setschedparam(thread_attrs, &param);
	if(ret < 0)
	{
		NxDbgMsg( NX_DBG_ERR, "Fail, pthread_attr_setschedparam\n" );
		return -1;
	}

	ret	= pthread_attr_getschedparam(thread_attrs, &param_res);
	NxDbgMsg( NX_DBG_INFO,"ret : %d ======priority : %d\n",ret, param_res.sched_priority );

	return 0;
}


int32_t NX_ThreadAttributeDestroy(pthread_attr_t *thread_attrs)
{
	int32_t res = 0;
	res = pthread_attr_destroy(thread_attrs);

	return res;
}


#endif //__NX_THREADUTILS__