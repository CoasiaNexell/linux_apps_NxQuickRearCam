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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/poll.h>

#include "NX_CGpioControl.h"

// #define NX_DBG_OFF

#ifdef NX_DTAG
#undef NX_DTAG
#endif
#define NX_DTAG "[NX_CGpioControl]"
#include <NX_DbgMsg.h>
//#include "../../../platform/common.h"

//------------------------------------------------------------------------------
//
//	NOTE.
//
//	1. GPIO Node
//	  -. /sys/class/gpio/gpio[num]
//
//	2. Create Node
//	  -. echo [gpio_num] > /sys/class/gpio/export
//
//	3. Remove Node
//	  -. echo [gpio_num] > /sys/class/gpio/unexport
//
//	4. GPIO Direction ( Default "in" )
//	  -. echo "in" > /sys/class/gpio/gpio/gpio[num]/direction
//	  -. echo "out" > /sys/class/gpio/gpio/gpio[num]/direction
//
//	5. Write Value
//	  -. echo 0 > /sys/class/gpio/gpio/gpio[num]/value
//	  -. echo 1 > /sys/class/gpio/gpio/gpio[num]/value
//
//	6. Read Value
//	  -. cat /sys/class/gpio/gpio/gpio[num]/value
//
//	7. Set Edge ( for poll() event ) ( Default "none" )
//	  -. echo "none" > /sys/class/gpio/gpio/gpio[num]/edge
//	  -. echo "riging" > /sys/class/gpio/gpio/gpio[num]/edge
//	  -. echo "falling" > /sys/class/gpio/gpio/gpio[num]/edge
//	  -. echo "both" > /sys/class/gpio/gpio/gpio[num]/edge
//
//	cf) depend on "cfg_main.h"
//		if you use edge flags, you must be disable key code. ( CFG_KEYPAD_KEY_BUTTON / CFG_KEYPAD_KEY_CODE )
//
//

//------------------------------------------------------------------------------
NX_CGpioControl::NX_CGpioControl()
	: m_iPort		( GPIO_ERROR )
	, m_iDirection	( GPIO_DIRECTION_IN )
	, m_iEdge		( GPIO_EDGE_NONE )
	, m_bInit		( false )
	, m_bReset		( false )
{

}

//------------------------------------------------------------------------------
NX_CGpioControl::~NX_CGpioControl()
{
	Deinit();
}

//------------------------------------------------------------------------------
int32_t NX_CGpioControl::Init( int32_t iGpio )
{
	//NxMsg( "%s()++\n", __FUNCTION__ );

	if( iGpio <= GPIO_ERROR || iGpio >= GPIO_MAX )
	{
		NxDbgMsg(NX_DBG_ERR, "Fail, Check GPIO Number. ( %d )\n", iGpio );
		//NxMsg( "%s()--\n", __FUNCTION__ );
		return -1;
	}

	char buf[64];
	snprintf( buf, sizeof(buf), "/sys/class/gpio/gpio%d", iGpio );

	if( !access(buf, F_OK) )
	{
		//NxDbgMsg(NX_DBG_ERR, "Fail, Already Initialize.\n" );
	}
	else
	{
		int32_t fd = open("/sys/class/gpio/export", O_WRONLY);

		if( 0 > fd )
		{
			NxDbgMsg(NX_DBG_ERR, "Fail, Open GPIO.\n" );
			//NxMsg( "%s()--\n", __FUNCTION__ );
			return -1;
		}

		int32_t len = snprintf( buf, sizeof(buf), "%d", iGpio );

		if( 0 > write(fd, buf, len) )
		{
			NxDbgMsg(NX_DBG_ERR, "Fail, Write GPIO.\n" );

			close( fd );
			return -1;
		}

		close( fd );
	}

	m_iPort		= iGpio;
	m_iDirection= GPIO_DIRECTION_IN;
	m_bInit		= true;

	pthread_mutex_init( &m_hLock, NULL );


	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CGpioControl::Deinit( void )
{

	if( false == m_bInit )
	{
		return -1;
	}

	int32_t fd = open("/sys/class/gpio/unexport", O_WRONLY);

	if( 0 > fd )
	{
		NxDbgMsg(NX_DBG_ERR, "Fail, Open GPIO.\n" );

		return -1;
	}

	char buf[64];
	int32_t len = snprintf( buf, sizeof(buf), "%d", m_iPort );

	if( 0 > write(fd, buf, len) )
	{
		close( fd );

		NxDbgMsg(NX_DBG_ERR, "Fail, Write GPIO.\n" );

		return -1;
	}

	close( fd );

	m_iPort		= GPIO_ERROR;
	m_iDirection= GPIO_DIRECTION_IN;
	m_iEdge		= GPIO_EDGE_NONE;
	m_bInit		= false;

	pthread_mutex_destroy( &m_hLock );

	NxDbgMsg(NX_DBG_ERR, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CGpioControl::SetDirection( int32_t iDirection )
{

	if( false == m_bInit )
	{
		return -1;
	}

	if( iDirection < GPIO_DIRECTION_IN || iDirection > GPIO_DIRECTION_OUT )
	{
		NxDbgMsg(NX_DBG_ERR, "Fail, Check Direction. ( %d )\n", iDirection );

		return -1;
	}

	char buf[64];
	snprintf( buf, sizeof(buf), "/sys/class/gpio/gpio%d/direction", m_iPort );

	int32_t fd = open( buf, O_WRONLY );
	if( 0 > fd )
	{
		NxDbgMsg(NX_DBG_ERR, "Fail, Open GPIO.\n" );

		return -1;
	}

	int32_t len = snprintf(buf, sizeof(buf), "%s", (iDirection == GPIO_DIRECTION_IN) ? "in" : "out");

	if( 0 > write( fd, buf, len ) ) {
		close(fd);

		NxDbgMsg(NX_DBG_ERR, "Fail, Write GPIO.\n" );

		return -1;
	}

	close( fd );


	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CGpioControl::SetValue( int32_t bValue )
{


	if( false == m_bInit )
	{
		return -1;
	}

	if( bValue < 0 || bValue > 1 )
	{
		NxDbgMsg(NX_DBG_ERR, "Fail, Check Value. ( %d )\n", bValue );

		return -1;
	}

	char buf[64];
	snprintf( buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", m_iPort );

	int32_t fd = open(buf, O_RDWR);
	if( 0 > fd )
	{
		NxDbgMsg(NX_DBG_ERR, "Fail, Open GPIO.\n" );

		return -1;
	}

	int32_t len = snprintf(buf, sizeof(buf), "%d", bValue);

	if( 0 > write(fd, buf, len) )
	{
		close(fd);

		NxDbgMsg(NX_DBG_ERR, "Fail, Write GPIO.\n" );

		return -1;
	}

	close( fd );


	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CGpioControl::GetValue( void )
{


	if( false == m_bInit )
	{
		return -1;
	}

	char buf[64];
	snprintf( buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", m_iPort );

	int32_t fd = open(buf, O_RDWR);
	if( 0 > fd )
	{
		NxDbgMsg(NX_DBG_ERR, "Fail, Open GPIO.\n" );

		return -1;
	}

	if( 0 > read(fd, buf, sizeof(buf)) )
	{
		close(fd);

		NxDbgMsg(NX_DBG_ERR, "Fail, Read GPIO.\n" );

		return -1;
	}

	close(fd);


	return atoi(buf);
}

//------------------------------------------------------------------------------
int32_t NX_CGpioControl::SetEdge( int32_t iEdge )
{
	NxDbgMsg(NX_DBG_INFO, "%s()++\n", __FUNCTION__ );

	if( false == m_bInit )
	{
		NxDbgMsg(NX_DBG_ERR, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	if( iEdge < GPIO_EDGE_NONE || iEdge > GPIO_EDGE_BOTH ) {
		NxDbgMsg(NX_DBG_ERR, "Fail, Check Edge.\n" );
		NxDbgMsg(NX_DBG_ERR, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	char buf[64];
	snprintf( buf, sizeof(buf), "/sys/class/gpio/gpio%d/edge", m_iPort );

	int32_t fd = open( buf, O_RDWR );
	if( 0 > fd )
	{
		NxDbgMsg(NX_DBG_ERR, "Fail, Open GPIO.\n" );
		NxDbgMsg(NX_DBG_ERR, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	int32_t len;
	if( iEdge == GPIO_EDGE_NONE )			len = snprintf( buf, sizeof(buf), "none" );
	else if( iEdge == GPIO_EDGE_FALLING )	len = snprintf( buf, sizeof(buf), "falling" );
	else if( iEdge == GPIO_EDGE_RIGING )	len = snprintf( buf, sizeof(buf), "rising" );
	else if( iEdge == GPIO_EDGE_BOTH )		len = snprintf( buf, sizeof(buf), "both" );

	if( 0 > write(fd, buf, len) )
	{
		close(fd);

		NxDbgMsg(NX_DBG_ERR, "Fail, Write GPIO.\n" );
		NxDbgMsg(NX_DBG_ERR, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	close(fd);

	m_iEdge = iEdge;

	NxDbgMsg(NX_DBG_INFO, "%s()--\n", __FUNCTION__ );
	return 0;
}

//------------------------------------------------------------------------------
int32_t NX_CGpioControl::WaitInterrupt( void )
{
	int32_t fd = 0;
	int32_t hPoll = 0;
	struct pollfd   pollEvent;
	int32_t bRun = true;
	char buf[64];

	pthread_mutex_lock( &m_hLock );
	m_bReset = false;
	pthread_mutex_unlock( &m_hLock );

	if( m_iDirection == GPIO_DIRECTION_OUT ) {
		NxDbgMsg(NX_DBG_ERR, "Fail, Check Direction.\n" );
		NxDbgMsg(NX_DBG_ERR, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	if( m_iEdge == GPIO_EDGE_NONE ) {
		NxDbgMsg(NX_DBG_ERR, "Fail, Check Edge.\n");
		NxDbgMsg(NX_DBG_ERR, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	snprintf( buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", m_iPort );

	if( 0 > (fd = open(buf, O_RDWR)) )
	{
		NxDbgMsg(NX_DBG_ERR, "Fail, Open GPIO.\n" );
		NxDbgMsg(NX_DBG_INFO, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	// Dummy Read.
	memset( buf, 0x00, sizeof(buf) );
	if( 0 > read(fd, buf, sizeof(buf)) ) {
		close(fd);

		NxDbgMsg(NX_DBG_ERR, "Fail, Read GPIO.\n" );
		NxDbgMsg(NX_DBG_INFO, "%s()--\n", __FUNCTION__ );
		return -1;
	}

	memset( &pollEvent, 0x00, sizeof(pollEvent) );
	pollEvent.fd	= fd;
	pollEvent.events= POLLPRI;

	while( bRun )
	{
		hPoll = poll( (struct pollfd*)&pollEvent, 1, 100 );

		if( hPoll < 0 )
		{
			close(fd);

			NxDbgMsg(NX_DBG_ERR, "Fail, poll().\n" );
			NxDbgMsg(NX_DBG_INFO, "%s()--\n", __FUNCTION__ );
			return -1;
		}
		else if( hPoll > 0 )
		{
			if( pollEvent.revents & POLLPRI )
			{
				close(fd);

				NxDbgMsg(NX_DBG_INFO, "%s()--\n", __FUNCTION__ );
				return 0;
			}
		}

		pthread_mutex_lock( &m_hLock );
		if( m_bReset ) bRun = false;
		pthread_mutex_unlock( &m_hLock );
	}

	close(fd);

	NxDbgMsg(NX_DBG_INFO, "%s()--\n", __FUNCTION__ );
	return -1;
}

//------------------------------------------------------------------------------
int32_t NX_CGpioControl::ResetInterrupt( void )
{
	NxDbgMsg(NX_DBG_INFO, "%s()++\n", __FUNCTION__ );

	pthread_mutex_lock( &m_hLock );
	m_bReset = true;
	pthread_mutex_unlock( &m_hLock );

	NxDbgMsg(NX_DBG_INFO, "%s()--\n", __FUNCTION__ );
	return 0;
}
