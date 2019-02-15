//------------------------------------------------------------------------------
//
//  Copyright (C) 2017 Nexell Co. All Rights Reserved
//  Nexell Co. Proprietary & Confidential
//
//  NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//  Module      :
//  File        :
//  Description :
//  Author      :
//  Export      :
//  History     :
//
//------------------------------------------------------------------------------

#ifndef __NX_CBASE_H__
#define __NX_CBASE_H__

#ifdef __cplusplus

#include <stdio.h>
#include <pthread.h>

//------------------------------------------------------------------------------
class NX_CMutex
{
public:
	NX_CMutex()
	{
		pthread_mutex_init( &m_hLock, NULL );
	}

	~NX_CMutex()
	{
		pthread_mutex_destroy( &m_hLock );
	}

public:
	void Lock()
	{
		pthread_mutex_lock( &m_hLock );
	}

	void Unlock()
	{
		pthread_mutex_unlock( &m_hLock );
	}

private:
	pthread_mutex_t	m_hLock;

private:
	NX_CMutex (const NX_CMutex &Ref);
	NX_CMutex &operator=(const NX_CMutex &Ref);
};

//------------------------------------------------------------------------------
class NX_CAutoLock
{
public:
	NX_CAutoLock( NX_CMutex *pLock )
		: m_pLock( pLock )
	{
		m_pLock->Lock();
	}

	~NX_CAutoLock()
	{
		m_pLock->Unlock();
	}

protected:
	NX_CMutex*	m_pLock;

private:
	NX_CAutoLock (const NX_CAutoLock &Ref);
	NX_CAutoLock &operator=(NX_CAutoLock &Ref);
};

#endif	//	__cplusplus

#endif	//	__NX_CBASE_H__
