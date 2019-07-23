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

#ifndef __NX_CBASEFILTER_H__
#define __NX_CBASEFILTER_H__

#ifdef __cplusplus

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
//#include <libcli.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>

#include "NX_ConfigTypes.h"
#include <NX_DbgMsg.h>

class NX_CBasePin;
class NX_CBaseInputPin;
class NX_CBaseOutputPin;
class NX_CBaseFilter;
class NX_CSample;
class NX_CRefClock;
class NX_CEventNotifier;

////////////////////////////////////////////////////////////////////////////////
//
//	NX_CMutex
//
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
	pthread_mutex_t		m_hLock;

private:
	NX_CMutex (const NX_CMutex &Ref);
	NX_CMutex &operator=(const NX_CMutex &Ref);
};


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CAutoLock
//
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
	NX_CMutex	*m_pLock;

private:
	NX_CAutoLock (const NX_CAutoLock &Ref);
	NX_CAutoLock &operator=(NX_CAutoLock &Ref);
};


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CRefClock
//
class NX_CRefClock
{
public:
	NX_CRefClock()
		: m_iDelta( 0 )
	{
	}
	virtual ~NX_CRefClock()
	{
	}

public:
	int64_t GetRefTime( void )
	{
		NX_CAutoLock lock( &m_hLock );
		struct timeval	tv;
		struct timezone	zv;
		gettimeofday( &tv, &zv );
		return ((int64_t)tv.tv_sec)*1000 + (int64_t)(tv.tv_usec/1000) + m_iDelta;
	}

	void	AdjustRefTime( int64_t iGap )
	{
		NX_CAutoLock lock( &m_hLock );
		m_iDelta += iGap;
	}

	void 	ResetRefTime( void )
	{
		NX_CAutoLock lock( &m_hLock );
		m_iDelta = 0;
	}

private:
	int64_t			m_iDelta;
	NX_CMutex 		m_hLock;

private:
	NX_CRefClock (const NX_CRefClock &Ref);
	NX_CRefClock &operator=(const NX_CRefClock &Ref);
};


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CSemaphore
//
class NX_CSemaphore
{
public:
	NX_CSemaphore( int32_t iMax, int32_t iInit )
		: m_iValue	( iInit )
		, m_iMax	( iMax )
		, m_iInit	( iInit )
		, m_bReset	( false )
	{
		pthread_cond_init	( &m_hCond,	NULL );
		pthread_mutex_init	( &m_hLock,NULL );
	}

	~NX_CSemaphore()
	{
		ResetSignal();
		pthread_cond_destroy( &m_hCond );
		pthread_mutex_destroy( &m_hLock );
	}

public:
	int32_t Post( void )
	{
		int32_t iRet = 0;
		pthread_mutex_lock( &m_hLock );
		if( m_iMax <= m_iValue ) {
			pthread_mutex_unlock( &m_hLock );
			return -1;
		}

		m_iValue++;
		pthread_cond_signal( &m_hCond );
		if( m_bReset || m_iValue <= 0 ) {
			iRet = -1;
		}
		pthread_mutex_unlock( &m_hLock );
		return iRet;
	}

	int32_t Pend( int32_t nSec = 1000000 )
	{
		int32_t iRet = 0;
		pthread_mutex_lock( &m_hLock );
		if( m_iValue == 0 && !m_bReset ) {
			int32_t iErr = 0;
			timespec timeSpec;
			do {
#ifdef __linux__
				clock_gettime( CLOCK_REALTIME, &timeSpec );
#else
				timeSpec.tv_nsec = GetTickCount();
				timeSpec.tv_sec  = timeSpec.tv_nsec / 1000;
				timeSpec.tv_nsec = timeSpec.tv_nsec * 1000;
#endif
				if( timeSpec.tv_nsec > (1000000 - nSec) ){
					timeSpec.tv_sec  += 1;
					timeSpec.tv_nsec -= (1000000 - nSec);
				} else {
					timeSpec.tv_nsec += nSec;
				}

				iErr = pthread_cond_timedwait( &m_hCond, &m_hLock, &timeSpec );

				if( m_bReset == true ) {
					iRet = -1;
					break;
				}
			} while( ETIMEDOUT == iErr );
			m_iValue--;
		}
		else if( m_iValue < 0 || m_bReset ) {
			iRet = -1;
		}
		else {
			m_iValue--;
		}
		pthread_mutex_unlock( &m_hLock );
		return iRet;
	}

	void ResetValue( void )
	{
		pthread_mutex_lock( &m_hLock );
		m_iValue = m_iInit;
		m_bReset = false;
		pthread_mutex_unlock( &m_hLock );
	}

	void ResetSignal( void )
	{
		pthread_mutex_lock( &m_hLock );
		m_bReset = true;
		for( int32_t i = 0; i < m_iMax; i++ )
			pthread_cond_signal( &m_hCond );
		pthread_mutex_unlock( &m_hLock );
	}

	int32_t GetValue( void )
	{
		int32_t iValue = 0;
		pthread_mutex_lock( &m_hLock );
		iValue = m_iValue;
		pthread_mutex_unlock( &m_hLock );
		return iValue;
	}

private:
	pthread_cond_t	m_hCond;
	pthread_mutex_t	m_hLock;

	int32_t			m_iValue;
	int32_t			m_iMax;
	int32_t			m_iInit;
	int32_t			m_bReset;

private:
	NX_CSemaphore (const NX_CSemaphore &Ref);
	NX_CSemaphore &operator=(const NX_CSemaphore &Ref);
};


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CBaseFilter
//
class NX_CBaseFilter
{
public:
	NX_CBaseFilter()
		: m_pOutFilter( NULL )
		, m_pRefClock( NULL )
		, m_pEventNotifier( NULL )
		, m_iMediaNum( 0 )
		, m_bRun( false )
	{
		memset( &m_FilterId,	0x00, sizeof(m_FilterId)	);
		memset( &m_FilterName,	0x00, sizeof(m_FilterName)	);
		memset( &m_FilterInfo,	0x00, sizeof(m_FilterInfo)	);

		for( int32_t i = 0; i < MAX_PIN_NUM; i++ )
		{
			memset( &m_MediaInfo[m_iMediaNum], 0x00, sizeof(NX_MEDIA_INFO) );
		}

		m_FilterInfo.pFilterId   = (char*)m_FilterId;
		m_FilterInfo.pFilterName = (char*)m_FilterName;
	}

	virtual ~NX_CBaseFilter()
	{
	}

public:
	virtual void*	FindInterface( const char* pFilterId, const char* pFilterName, const char* pInterfaceId ) = 0;
	virtual NX_CBasePin* FindPin( int32_t iDirection, int32_t iIndex ) = 0;

	virtual int32_t	Run( void )		= 0;
	virtual int32_t	Stop( void )	= 0;
	virtual int32_t	Pause( int32_t )	= 0;

	virtual void	SetFilterInfo( NX_FILTER_INFO *pInfo )
	{
		m_FilterInfo = *pInfo;
	}

	virtual void	GetFilterInfo( NX_FILTER_INFO *ppInfo )
	{
		(void)ppInfo;
		//ppInfo = &m_FilterInfo;
	}

	virtual void	SetMediaInfo( NX_MEDIA_INFO *pInfo )
	{
		memcpy( &m_MediaInfo[m_iMediaNum], pInfo, sizeof(NX_MEDIA_INFO) );
		m_iMediaNum++;
	}

	virtual int32_t	GetMediaInfo( NX_MEDIA_INFO **ppInfo )
	{
		*ppInfo = &m_MediaInfo[0];
		return m_iMediaNum;
	}

	virtual void	SetFilterId( const char *pFilterId )
	{
		snprintf( (char*)m_FilterId, sizeof(m_FilterId), "%s", pFilterId );
	}

	virtual int8_t*	GetFilterId( void )
	{
		return m_FilterId;
	}

	virtual void	SetFilterName( const char *pFilterName )
	{
		snprintf( (char*)m_FilterName, sizeof(m_FilterName), "%s", pFilterName );
	}

	virtual int8_t*	GetFilterName( void )
	{
		return m_FilterName;
	}

	virtual void	SetRefClock( NX_CRefClock *pClock )
	{
		m_pRefClock = pClock;
	}

	virtual void	SetEventNotifier( NX_CEventNotifier *pNotifier )
	{
		m_pEventNotifier = pNotifier;
	}

private:
	enum { MAX_PIN_NUM = 16, MAX_STRING_NUM = 256 };

protected:
	NX_CBaseFilter*		m_pOutFilter;
	NX_CRefClock*		m_pRefClock;
	NX_CEventNotifier*	m_pEventNotifier;

	int8_t				m_FilterId[MAX_STRING_NUM];
	int8_t				m_FilterName[MAX_STRING_NUM];

	NX_FILTER_INFO		m_FilterInfo;
	NX_MEDIA_INFO		m_MediaInfo[MAX_PIN_NUM];

	int32_t				m_iMediaNum;

	int32_t				m_bRun;

private:
	NX_CBaseFilter (const NX_CBaseFilter &Ref);
	NX_CBaseFilter &operator=(const NX_CBaseFilter &Ref);
};


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CBasePin
//
class NX_CBasePin
{
public:
	NX_CBasePin()
		: m_pOwnerFilter( NULL )
		, m_bActive( false )
	{

	}

	virtual ~NX_CBasePin() {}

public:
	virtual void	SetOwner( NX_CBaseFilter *pOwner )
	{
		m_pOwnerFilter = pOwner;
	}

	virtual void	Active( void )
	{
		NX_CAutoLock lock( &m_hLock );
		m_bActive = true;
	}

	virtual void	Inactive( void )
	{
		NX_CAutoLock lock( &m_hLock );
		m_bActive = false;
	}

	virtual int32_t	IsActive( void )
	{
		NX_CAutoLock lock( &m_hLock );
		return m_bActive;
	}

	virtual void	SetPinInfo( NX_PIN_INFO *pInfo )
	{
		m_PinInfo = *pInfo;
	}

	virtual void	GetPinInfo( NX_PIN_INFO **ppInfo )
	{
		*ppInfo = &m_PinInfo;
	}

protected:
	NX_CBaseFilter*		m_pOwnerFilter;
	NX_PIN_INFO			m_PinInfo;

private:
	int32_t				m_bActive;
	NX_CMutex			m_hLock;

private:
	NX_CBasePin (const NX_CBasePin &Ref);
	NX_CBasePin &operator=(const NX_CBasePin &Ref);
};


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CBaseInputPin
//
class NX_CBaseInputPin
	: public NX_CBasePin
{
public:
	NX_CBaseInputPin()
	{
		memset( &m_PinInfo, 0x00, sizeof(m_PinInfo) );
		m_PinInfo.iDirection = NX_PIN_INPUT;
	}

	virtual ~NX_CBaseInputPin() {}

public:
	virtual int32_t Receive( NX_CSample *pSample )				= 0;
	virtual int32_t GetSample( NX_CSample **ppSample )			= 0;
	virtual int32_t Flush( void )								= 0;

	virtual int32_t PinNegotiation( NX_CBaseOutputPin *pOutPin )= 0;

	virtual void	SetMediaInfo( NX_MEDIA_INFO *pInfo )
	{
		m_pOwnerFilter->SetMediaInfo( pInfo );
	}

private:
	NX_CBaseInputPin (const NX_CBaseInputPin &Ref);
	NX_CBaseInputPin &operator=(const NX_CBaseInputPin &Ref);
};


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CBaseOutputPin
//
class NX_CBaseOutputPin
	: public NX_CBasePin
{
public:
	NX_CBaseOutputPin()
		: m_pPartnerPin( NULL )
	{
		memset( &m_PinInfo, 0x00, sizeof(m_PinInfo) );
		m_PinInfo.iDirection = NX_PIN_OUTPUT;
	}

	virtual ~NX_CBaseOutputPin() {}

public:
	virtual void	Connect( NX_CBaseInputPin *pInPin )
	{
		if( NULL != pInPin && NULL == m_pPartnerPin ) {
			m_pPartnerPin = pInPin;
		}
	}

	virtual void 	Disconnect( void )
	{
		if( m_pPartnerPin ) {
			m_pPartnerPin = NULL;
		}
	}

	virtual int32_t	IsConnected( void )
	{
		return (NULL != m_pPartnerPin) ? true : false;
	}

	virtual int32_t Deliver( NX_CSample *pSample )
	{
		if( true == IsConnected() && true == IsActive() ) {
			//NxDbgMsg( NX_DBG_VBS, "%s : line %d\n",  __func__, __LINE__);
			return m_pPartnerPin->Receive( pSample );
		}
		return -1;
	}

	virtual int32_t GetDeliverySample( NX_CSample **ppSample )	= 0;
	virtual int32_t ReleaseSample( NX_CSample *pSample )		= 0;

	virtual int32_t	GetMediaInfo( NX_MEDIA_INFO **ppInfo )
	{
		return m_pOwnerFilter->GetMediaInfo( ppInfo );
	}

protected:
	NX_CBaseInputPin*	m_pPartnerPin;

private:
	NX_CBaseOutputPin (const NX_CBaseOutputPin &Ref);
	NX_CBaseOutputPin &operator=(const NX_CBaseOutputPin &Ref);
};


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CSample
//
class NX_CSample
{
public:
	NX_CSample()
	: m_iRefCount( 0 )
	, m_pOwner( NULL )
	, m_iMediaType( 0 )
	, m_pBuf( NULL )
	, m_iBufSize( 0 )
	, m_iActualBufSize( 0 )
	, m_bSyncPoint( false )
	, m_iTimeStamp( -1 )
	, m_iIndex( 0 )
	, m_pPrivate( NULL )
	{
	}

	virtual ~NX_CSample() { }

public:
	virtual int32_t	Lock( void )
	{
		NX_CAutoLock lock( &m_hLock );
		m_iRefCount++;

		return m_iRefCount;
	}

	virtual int32_t	Unlock( void )
	{
		NX_CAutoLock lock( &m_hLock );
		if( m_iRefCount > 0 )
		{
			m_iRefCount--;
			if( m_iRefCount == 0 )
			{
				m_pOwner->ReleaseSample( this );
			}
		}
		return m_iRefCount;
	}

	virtual void	SetOwner( NX_CBaseOutputPin *pOwner )
	{
		m_pOwner = pOwner;
	}

	virtual void	SetBuffer( void *pBuf, int32_t iBufSize )
	{
		m_pBuf		= pBuf;
		m_iBufSize	= iBufSize;
	}

	virtual int32_t GetBuffer( void **ppBuf, int32_t *iBufSize )
	{
		if( NULL != m_pBuf )
		{
			*ppBuf		= m_pBuf;
			*iBufSize	= m_iBufSize;
			return 0;
		}
		*ppBuf		= NULL;
		iBufSize	= 0;
		return -1;
	}

	virtual void	SetActualBufSize( int32_t iSize )
	{
		m_iActualBufSize = iSize;
	}

	virtual int32_t	GetActualBufSize( void )
	{
		return m_iActualBufSize;
	}

	virtual void	SetSyncPoint( int32_t bSyncPoint )
	{
		m_bSyncPoint = bSyncPoint;
	}

	virtual int32_t	GetSyncPoint( void )
	{
		return m_bSyncPoint;
	}

	virtual void 	SetTimeStamp( int64_t iTimeStamp )
	{
		m_iTimeStamp = iTimeStamp;
	}

	virtual int64_t	GetTimeStamp( void )
	{
		return m_iTimeStamp;
	}

	virtual void SetIndex( int32_t iIndex )
	{
		m_iIndex = iIndex;
	}

	virtual int32_t GetIndex( void )
	{
		return m_iIndex;
	}

	virtual void SetPrivate( void *pPrivate )
	{
		m_pPrivate = pPrivate;
	}

	virtual void* GetPrivate( void )
	{
		return m_pPrivate;
	}

	int32_t				m_iRefCount;

private:
	NX_CBaseOutputPin*	m_pOwner;
	//int32_t				m_iRefCount;
	int32_t				m_iMediaType;
	void*				m_pBuf;
	int32_t				m_iBufSize;
	int32_t				m_iActualBufSize;
	int32_t				m_bSyncPoint;
	int64_t				m_iTimeStamp;
	int32_t				m_iIndex;
	void*				m_pPrivate;
	NX_CMutex			m_hLock;

private:
	NX_CSample (const NX_CSample &Ref);
	NX_CSample &operator=(const NX_CSample &Ref);
};


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CSampleQueue
//
class NX_CSampleQueue
{
public:
	NX_CSampleQueue( int32_t iMaxQueueCount )
		: m_iHeadIndex		( 0 )
		, m_iTailIndex		( 0 )
		, m_iSampleCount	( 0 )
		, m_iMaxQueueCount	( iMaxQueueCount )
	{
		m_ppSamplePool = (NX_CSample **)malloc( sizeof(NX_CSample*) * m_iMaxQueueCount );
	}

	~NX_CSampleQueue()
	{
		if( m_ppSamplePool )
			free( m_ppSamplePool );
	}

public:
	int32_t PushSample( NX_CSample *pSample )
	{
		NX_CAutoLock lock( &m_hLock );
		if( m_iMaxQueueCount <= m_iSampleCount )
			return -1;

		m_ppSamplePool[m_iTailIndex] = pSample;
		m_iTailIndex = ( m_iTailIndex + 1) % m_iMaxQueueCount;
		m_iSampleCount++;

		return 0;
	}

	int32_t PopSample( NX_CSample **ppSample )
	{
		NX_CAutoLock lock( &m_hLock );

		if( 0 >= m_iSampleCount ) {
			*ppSample = NULL;
			return -1;
		}

		*ppSample = m_ppSamplePool[m_iHeadIndex];
		m_iHeadIndex = (m_iHeadIndex + 1) % m_iMaxQueueCount;
		m_iSampleCount--;

		return 0;
	}

	void ResetValue( void )
	{
		NX_CAutoLock lock( &m_hLock );
		m_iHeadIndex = 0;
		m_iTailIndex = 0;
		m_iSampleCount = 0;
	}

	int32_t GetSampleCount( void )
	{
		NX_CAutoLock lock( &m_hLock );
		return m_iSampleCount;
	}

	int32_t IsEmpty( void )
	{
		NX_CAutoLock lock( &m_hLock );
		return (0 == m_iSampleCount) ? true : false;
	}

	int32_t IsFull( void )
	{
		NX_CAutoLock lock( &m_hLock );
		return (m_iMaxQueueCount == m_iSampleCount) ? true : false;
	}

private:
	NX_CSample**	m_ppSamplePool;
	NX_CMutex		m_hLock;
	int32_t			m_iHeadIndex, m_iTailIndex, m_iSampleCount;
	int32_t			m_iMaxQueueCount;

private:
	NX_CSampleQueue (const NX_CSampleQueue &Ref);
	NX_CSampleQueue &operator=(const NX_CSampleQueue &Ref);
};


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CQueue
//
class NX_CQueue
{
public:
	NX_CQueue()
		: m_iHeadIndex		( 0 )
		, m_iTailIndex		( 0 )
		, m_iBufCount		( 0 )
	{
	}

	virtual ~NX_CQueue()
	{
	}

public:
	int32_t Push( int32_t iBuf )
	{
		NX_CAutoLock lock( &m_hLock );
		if( MAX_QUEUE_COUNT <= m_iBufCount )
			return -1;

		m_iBuf[m_iTailIndex]	= iBuf;

		m_iTailIndex = ( m_iTailIndex + 1) % MAX_QUEUE_COUNT;
		m_iBufCount++;

		return 0;
	}

	int32_t Pop( int32_t *iBuf )
	{
		NX_CAutoLock lock( &m_hLock );

		if( 0 >= m_iBufCount ) {
			*iBuf = 0;
			return -1;
		}

		*iBuf 		= m_iBuf[m_iHeadIndex];

		m_iHeadIndex= (m_iHeadIndex + 1) % MAX_QUEUE_COUNT;
		m_iBufCount--;

		return 0;
	}

	int32_t Push( void *pBuf )
	{
		NX_CAutoLock lock( &m_hLock );
		if( MAX_QUEUE_COUNT <= m_iBufCount )
			return -1;

		m_pBuf[m_iTailIndex]	= pBuf;

		m_iTailIndex = ( m_iTailIndex + 1) % MAX_QUEUE_COUNT;
		m_iBufCount++;

		return 0;
	}

	int32_t Pop( void **ppBuf )
	{
		NX_CAutoLock lock( &m_hLock );

		if( 0 >= m_iBufCount ) {
			*ppBuf = NULL;
			return -1;
		}

		*ppBuf 		= m_pBuf[m_iHeadIndex];

		m_iHeadIndex= (m_iHeadIndex + 1) % MAX_QUEUE_COUNT;
		m_iBufCount--;

		return 0;
	}

	int32_t Push( void *pBuf, int32_t iBufSize )
	{
		NX_CAutoLock lock( &m_hLock );
		if( MAX_QUEUE_COUNT <= m_iBufCount )
			return -1;

		m_pBuf[m_iTailIndex]	= pBuf;;
		m_pBufSize[m_iTailIndex]= iBufSize;

		m_iTailIndex = ( m_iTailIndex + 1) % MAX_QUEUE_COUNT;
		m_iBufCount++;

		return 0;
	}

	int32_t Pop( void **ppBuf, int32_t *iBufSize )
	{
		NX_CAutoLock lock( &m_hLock );

		if( 0 >= m_iBufCount ) {
			*ppBuf = NULL;
			return -1;
		}

		*ppBuf 		= m_pBuf[m_iHeadIndex];
		*iBufSize	= m_pBufSize[m_iHeadIndex];

		m_iHeadIndex= (m_iHeadIndex + 1) % MAX_QUEUE_COUNT;
		m_iBufCount--;

		return 0;
	}

	void Reset( void )
	{
		NX_CAutoLock lock( &m_hLock );
		m_iHeadIndex = 0;
		m_iTailIndex = 0;
		m_iBufCount = 0;
	}

	int32_t GetCount( void )
	{
		NX_CAutoLock lock( &m_hLock );
		return m_iBufCount;
	}

	int32_t IsEmpty( void )
	{
		NX_CAutoLock lock( &m_hLock );
		return (0 == m_iBufCount) ? true : false;
	}

private:
	enum { MAX_QUEUE_COUNT = 128 };

	int32_t		m_iBuf[MAX_QUEUE_COUNT];
	void*		m_pBuf[MAX_QUEUE_COUNT];
	int32_t		m_pBufSize[MAX_QUEUE_COUNT];

	NX_CMutex	m_hLock;
	int32_t		m_iHeadIndex, m_iTailIndex, m_iBufCount;

private:
	NX_CQueue (const NX_CQueue &Ref);
	NX_CQueue &operator=(const NX_CQueue &Ref);
};


////////////////////////////////////////////////////////////////////////////////
//
//	NX_IEventReceiver
//
class NX_IEventReceiver
{
public:
	NX_IEventReceiver() {}
	virtual ~NX_IEventReceiver() {}

public:
	virtual void ProcessEvent( uint32_t iEventCode, void *pData, uint32_t iDataSize ) = 0;

private:
	NX_IEventReceiver (const NX_IEventReceiver &Ref);
	NX_IEventReceiver &operator=(const NX_IEventReceiver &Ref);
};


////////////////////////////////////////////////////////////////////////////////
//
//	NX_CEventNotifier
//
class NX_CEventNotifier
{
public:
	NX_CEventNotifier()
		: m_pEventReceiver( NULL )
	{

	}
	virtual ~NX_CEventNotifier() {}

public:
	virtual void SendEvent( uint32_t iEventCode, void *pData, uint32_t iDataSize )
	{
		NX_CAutoLock lock( &m_hLock );
		if( m_pEventReceiver ) {
			m_pEventReceiver->ProcessEvent( iEventCode, pData, iDataSize );
		}
	}

	virtual void SetEventReceiver( NX_IEventReceiver *pReceiver )
	{
		m_pEventReceiver = pReceiver;
	}

private:
	NX_IEventReceiver*	m_pEventReceiver;
	NX_CMutex			m_hLock;

private:
	NX_CEventNotifier (const NX_CEventNotifier &Ref);
	NX_CEventNotifier &operator=(const NX_CEventNotifier &Ref);
};


#endif	// __cplusplus

#endif	// __NX_CBASEFILTER_H__
