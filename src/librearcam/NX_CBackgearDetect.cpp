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
//	Module		: Back Gear Detect Module for Auto Mobile
//	File		: NX_CBackgearDetect.xxx
//	Description	:
//	Author		: SeongO Park (ray@nexell.co.kr)
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

// #define NX_DBG_OFF

#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_CBackgearDetect]"

#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

#include <NX_BackGearDetect.h>

#include "NX_CGpioControl.h"
#include <NX_DbgMsg.h>

#include "NX_ThreadUtils.h"

//#include "../../../platform/common.h"


class NX_CBackgearDetect
{
public:
	void RegEventCallback(void*,  void (*cbFunc)(void *,  int32_t) );
	int32_t StartService( int32_t nGpio, int32_t nChkDelay, int32_t iDetectDelay );
	void StopService();

	NX_CBackgearDetect();
	~NX_CBackgearDetect();

	static NX_CBackgearDetect *GetInstance();
	static NX_CBackgearDetect *m_pSingleTone;

private:
	static void*	ThreadStub( void *pObj );
	void 			ThreadProc();


private:
	//
	// Hardware Depend Parameter
	//

	static NX_CBackgearDetect* m_pBackgearDetect;

	NX_CGpioControl*	m_pGpioControl;
	int32_t				m_iCurGearStatus;

	int32_t				m_bExitLoop;
	int32_t				m_CheckDealy;	// usec

	//	for Callback Function
	void*				m_pCbAppData;
	void				(*m_EventCallBack)(void *, int32_t);

	//	GPIO Access Mutex
	pthread_mutex_t		m_hGpioMutex;

	// thread
	pthread_t		m_hThread;
	pthread_attr_t thread_attrs;

	int32_t			m_bThreadRun;


private:
	NX_CBackgearDetect (const NX_CBackgearDetect &Ref);
	NX_CBackgearDetect &operator=(const NX_CBackgearDetect &Ref);
};


//------------------------------------------------------------------------------
NX_CBackgearDetect::NX_CBackgearDetect()
	: m_pGpioControl ( NULL )
	, m_bExitLoop ( false )
	, m_CheckDealy ( 10 )
	, m_pCbAppData ( NULL )
	, m_EventCallBack ( NULL )
	, m_bThreadRun( false )
{
	pthread_mutex_init( &m_hGpioMutex, NULL );
}

//------------------------------------------------------------------------------
NX_CBackgearDetect::~NX_CBackgearDetect()
{
	m_pGpioControl->ResetInterrupt();
	pthread_join( m_hThread, NULL );

	pthread_mutex_lock(&m_hGpioMutex);
	if( m_pGpioControl )
	{
		m_pGpioControl->Deinit();
		delete m_pGpioControl;
	}
	pthread_mutex_unlock(&m_hGpioMutex);
	pthread_mutex_destroy( &m_hGpioMutex );
}

void NX_CBackgearDetect::RegEventCallback( void *pAppData, void (*cbFunc)(void *, int32_t) )
{
	m_pCbAppData = pAppData;
	m_EventCallBack = cbFunc;
}

int32_t NX_CBackgearDetect::StartService( int32_t nGpio, int32_t nChkDelay, int32_t iDetectDelay )
{
	(void)iDetectDelay;
	m_bExitLoop = false;

	pthread_mutex_lock(&m_hGpioMutex);
	if( m_pGpioControl )
		delete m_pGpioControl;
	m_pGpioControl = new NX_CGpioControl();

	m_CheckDealy = nChkDelay;
	m_pGpioControl->Init( nGpio );
	m_pGpioControl->SetDirection( GPIO_DIRECTION_IN );
	m_pGpioControl->SetEdge( GPIO_EDGE_BOTH );
	pthread_mutex_unlock(&m_hGpioMutex);

	if(m_bThreadRun == false)
	{
		m_bThreadRun = true;
#ifdef ADJUST_THREAD_PRIORITY
		NX_AdjustThreadPriority(&thread_attrs, SCHED_RR, THREAD_PRIORITY);

		if( 0 > pthread_create( &this->m_hThread, &thread_attrs, this->ThreadStub, this ) ) {
				NxDbgMsg( NX_DBG_ERR, "Fail, Create Thread.\n" );
				return -1;
		}
#else
		if( 0 > pthread_create( &this->m_hThread, NULL, this->ThreadStub, this ) ) {
				NxDbgMsg( NX_DBG_ERR, "Fail, Create Thread.\n" );
				return -1;
		}
#endif
	}

	return 0;
}

void NX_CBackgearDetect::StopService()
{
	m_bExitLoop = true;

#ifdef ADJUST_THREAD_PRIORITY
	NX_ThreadAttributeDestroy(&thread_attrs);
#endif
	pthread_join( m_hThread, NULL );

	m_bThreadRun = false;

	pthread_mutex_lock(&m_hGpioMutex);
	if( m_pGpioControl )
	{
		m_pGpioControl->Deinit();
		delete m_pGpioControl;
	}
	pthread_mutex_unlock(&m_hGpioMutex);
}

void  NX_CBackgearDetect::ThreadProc()
{
	int32_t readStatus;
	m_iCurGearStatus = -1;

	do{
		pthread_mutex_lock(&m_hGpioMutex);
		readStatus = m_pGpioControl->GetValue();
		pthread_mutex_unlock(&m_hGpioMutex);
		if( m_iCurGearStatus != readStatus )
		{
			if(m_EventCallBack)
			{
				m_EventCallBack(m_pCbAppData, readStatus );
			}

			NxDbgMsg(NX_DBG_INFO, "Backgear status changed (gpio value : %d-->%d)\n", m_iCurGearStatus, readStatus );
		}
		m_iCurGearStatus = readStatus;
		usleep( m_CheckDealy * 1000 );

	}while( !m_bExitLoop );

}

//------------------------------------------------------------------------------
void *NX_CBackgearDetect::ThreadStub( void *pObj )
{
	if( NULL != pObj ) {
		((NX_CBackgearDetect*)pObj)->ThreadProc();
	}

	return (void*)0xDEADDEAD;
}


//
//
//	Make Singletone Instance
//
//
NX_CBackgearDetect *NX_CBackgearDetect::m_pSingleTone = NULL;

NX_CBackgearDetect *NX_CBackgearDetect::GetInstance()
{
	if( NULL == m_pSingleTone )
	{
		m_pSingleTone = new NX_CBackgearDetect();
	}
	return m_pSingleTone;
}



//
//		External Interface
//
int32_t NX_StartBackGearDetectService( int32_t nGpio, int32_t nChkDelay )
{
	int32_t res = 0;
	NX_CBackgearDetect *pInst = (NX_CBackgearDetect *)NX_CBackgearDetect::GetInstance();
	res = pInst->StartService(nGpio, nChkDelay, 0);
	return res;
}

void NX_StopBackGearDetectService()
{
	NX_CBackgearDetect *pInst = (NX_CBackgearDetect *)NX_CBackgearDetect::GetInstance();
	pInst->StopService();
}

void NX_RegisterBackGearEventCallBack(void *pAppData, void (*callback)(void *,  int32_t))
{
	NX_CBackgearDetect *pInst = (NX_CBackgearDetect *)NX_CBackgearDetect::GetInstance();
	pInst->RegEventCallback( pAppData, callback );
}
