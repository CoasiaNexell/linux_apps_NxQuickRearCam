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

#ifndef __CNX_BASECLASS_H__
#define __CNX_BASECLASS_H__

#include <stdint.h>
#include <pthread.h>

class CNX_Thread{
public:
	CNX_Thread()
	{
		m_IsRunning = false;
	}
	virtual ~CNX_Thread(){
		Stop();
	}
	virtual int32_t Start()
	{
		if( 0 != pthread_create( &m_hThread, NULL, CNX_Thread::ThreadStub, this ) )
		{
			m_IsRunning = false;
			return -1;
		}
		m_IsRunning = true;
		return 0;
	}
	virtual void Stop()
	{
		if( m_IsRunning )
		{
			pthread_join( m_hThread, NULL );
			//	consider pthread_tryjoin_np() function for dead lock
		}
	}
	virtual void ThreadProc() = 0;
	static void *ThreadStub( void *arg )
	{
		CNX_Thread *pObj = (CNX_Thread*) arg;
		pObj->ThreadProc();
		return (void*)0xDeadC0de;
	}
protected:
	bool		m_IsRunning;
	pthread_t	m_hThread;
};


class CNX_AutoLock{
public:
	CNX_AutoLock( pthread_mutex_t *pLock ) :m_pLock(pLock)
	{
		pthread_mutex_lock( m_pLock );
	}
	~CNX_AutoLock()
	{
		pthread_mutex_unlock(m_pLock);
	}
protected:
	pthread_mutex_t	*m_pLock;
private:
	CNX_AutoLock (const CNX_AutoLock &Ref);
	CNX_AutoLock &operator=(CNX_AutoLock &Ref);
};

#endif	// __CNX_BASECLASS_H__
