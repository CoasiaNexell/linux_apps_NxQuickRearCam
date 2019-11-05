#include <stdio.h>
#include <poll.h>
#include <stdint.h>	//	intXX_t
#include <fcntl.h>	//	O_RDWR
#include <string.h>	//	memset
#include <unistd.h>
#include <stddef.h>	//	struct sockaddr_un
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#include "CNX_BaseClass.h"
#include "NX_CCommand.h"

#define POLL_TIMEOUT_MS		(2000)

class NX_CCommand : protected CNX_Thread
{
public:
	void RegEventCallback( void (*cbFunc)( int32_t) );
	void StartService(char *);
	void StopService();

	NX_CCommand();
	~NX_CCommand();

	//  Implementation Pure Virtual Function
	virtual void  ThreadProc();

private:
	//
	// Hardware Depend Parameter
	//

	static NX_CCommand* m_pCommandServer;

	int32_t				m_bExitLoop;
	int32_t 			fd_gpio;
	int32_t				fd_cmd;

	struct pollfd poll_fds;
	int32_t poll_ret;
	uint8_t rx_buf[256];

	char *m_pStopCmdFileName;
	char *m_pStatusCmdFileName;

	//	for Callback Function
	void				(*m_EventCallBack)(int32_t);

#ifdef ANDROID
	int32_t m_iGotStopCmd;
#endif

private:
	NX_CCommand (const NX_CCommand &Ref);
	NX_CCommand &operator=(const NX_CCommand &Ref);
};

//------------------------------------------------------------------------------
NX_CCommand::NX_CCommand()
	:m_bExitLoop ( false )
	, m_EventCallBack ( NULL )
{

}

//------------------------------------------------------------------------------
NX_CCommand::~NX_CCommand()
{
	pthread_join( m_hThread, NULL );
}

void NX_CCommand::RegEventCallback( void (*cbFunc)(int32_t) )
{
	m_EventCallBack = cbFunc;
}

void NX_CCommand::StartService(char *m_pCtrlFileName)
{
	m_bExitLoop = false;

	m_pStopCmdFileName = m_pCtrlFileName;

	Start();
}

void NX_CCommand::StopService()
{
	m_bExitLoop = true;
	Stop();
	close( fd_cmd );
	remove(m_pStopCmdFileName);
}

void  NX_CCommand::ThreadProc()
{
#ifndef ANDROID
	usleep(1500000);   //for mknod after tmpfs load
	

	if( 0 == access(m_pStopCmdFileName, F_OK))
	{
		remove(m_pStopCmdFileName);
	}

	int ret = mknod(m_pStopCmdFileName, S_IFIFO | 0666, 0 );
	printf("================mknod ret : %d\n", ret);
	fd_cmd  = open( m_pStopCmdFileName, O_RDWR );

	do{

		memset( &poll_fds, 0, sizeof(poll_fds) );
		poll_fds.fd = fd_cmd;
		poll_fds.events = POLLIN;

		poll_ret = poll( &poll_fds, 2, POLL_TIMEOUT_MS );
		if( poll_ret == 0 )
		{
			//printf("Poll Timeout\n");
			continue;
		}
		else if( 0 > poll_ret )
		{
			printf("Poll Error !!!\n");
		}
		else
		{
			int read_size;

			if( poll_fds.revents & POLLIN )
			{
				memset( rx_buf, 0, sizeof(rx_buf) );
				read_size = read( fd_cmd, rx_buf, sizeof(rx_buf));
				printf("CMD : Size = %d, %s\n", read_size, rx_buf);

				if(!strncmp((const char*)rx_buf, "quick_stop", sizeof("quick_stop")))
				{
					m_EventCallBack(STOP);
					m_bExitLoop = 1;
				}

			}
		}

	}while( !m_bExitLoop );

#else

	usleep(5000000);
	m_iGotStopCmd = 0;
	while(!m_bExitLoop){
		if( 0 > access(m_pStopCmdFileName, F_OK))
		{
			usleep(100000);
			continue;
		}else
		{
			if(m_iGotStopCmd != 1)
			{
				m_EventCallBack(STOP);
			}
			m_iGotStopCmd = 1;
			m_bExitLoop = true;
		}
		usleep(100000);
	}
#endif

}

//
//		External Interface
//
void* NX_GetCommandHandle()
{
	NX_CCommand *pInst = new NX_CCommand();

	return (void*)pInst;
}

int32_t NX_StartCommandService(void *pObj, char *m_pCtrlFileName)
{
	NX_CCommand *pInst = (NX_CCommand *)pObj;

	pInst->StartService(m_pCtrlFileName);
	return 0;
}

void NX_StopCommandService(void *pObj, char *m_pCtrlFileName)
{
	int32_t fd_status;

	NX_CCommand *pInst = (NX_CCommand*)pObj;

	pInst->StopService();

#ifndef ANDROID
	delete pInst;

	fd_status = open(m_pCtrlFileName, O_RDWR);

	write(fd_status, "stopped", sizeof("stopped"));

	close( fd_status );
#else
	usleep(1000000);
	if( 0 == access(m_pCtrlFileName, F_OK))
	{
		remove(m_pCtrlFileName);
	}

	int ret = mknod(m_pCtrlFileName, S_IFIFO | 0666, 0 );
	printf("================mknod ret : %d  : %s\n", ret, m_pCtrlFileName);

	usleep(30000000);

	remove(m_pCtrlFileName);

#endif
}

void NX_RegisterCommandEventCallBack(void *pObj, void (*callback)( int32_t))
{
	NX_CCommand *pInst = (NX_CCommand*)pObj;
	pInst->RegEventCallback( callback );
}




