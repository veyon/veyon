/*
 * system_service.h - convenient class for using app as system-service
 *           
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *  
 * This file is part of iTALC - http://italc.sourceforge.net
 * This file is part of LUPUS - http://lupus.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _SYSTEM_SERVICE_H
#define _SYSTEM_SERVICE_H


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef BUILD_WIN32
#include <windows.h>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

#define WTS_SESSION_LOGON 0x5
#define WTS_SESSION_LOGOFF 0x6
#define WTS_CURRENT_SERVER_HANDLE 0

typedef enum _WTS_INFO_CLASS
{
	WTSInitialProgram       = 0,
	WTSApplicationName      = 1,
	WTSWorkingDirectory     = 2,
	WTSOEMId                = 3,
	WTSSessionId            = 4,
	WTSUserName             = 5,
	WTSWinStationName       = 6,
	WTSDomainName           = 7,
	WTSConnectState         = 8,
	WTSClientBuildNumber    = 9,
	WTSClientName           = 10,
	WTSClientDirectory      = 11,
	WTSClientProductId      = 12,
	WTSClientHardwareId     = 13,
	WTSClientAddress        = 14,
	WTSClientDisplay        = 15,
	WTSClientProtocolType   = 16,
	WTSIdleTime             = 17,
	WTSLogonTime            = 18,
	WTSIncomingBytes        = 19,
	WTSOutgoingBytes        = 20,
	WTSIncomingFrames       = 21,
	WTSOutgoingFrames       = 22,
	WTSClientInfo           = 23,
	WTSSessionInfo          = 24 
} WTS_INFO_CLASS;

typedef BOOL(WINAPI *PFN_WTSQuerySessionInformation)( HANDLE, DWORD,
					WTS_INFO_CLASS, LPTSTR*, DWORD* );
extern PFN_WTSQuerySessionInformation pfnWTSQuerySessionInformation;

typedef void(WINAPI *PFN_WTSFreeMemory)( PVOID );
extern PFN_WTSFreeMemory pfnWTSFreeMemory;

#endif

#include <QtCore/QString>
#include <QtCore/QThread>


class systemService
{
public:
	typedef int( * service_main )( systemService * );

	systemService( const QString & _service_name,
			const QString & _service_arg = QString::null,
			const QString & _service_display_name = QString::null,
			const QString & _service_dependencies = QString::null,
			service_main _sm = NULL,
			int _argc = 0,
			char * * _argv = NULL );
	//~systemService( void );

	// install service - will start at next reboot
	bool install( void );

	// unregister service
	bool remove( void );

	// re-install service
	bool reinstall( void )
	{
		return( remove() && install() );
	}

	// start service if possible
	bool start( void );

	// stop service if possible
	bool stop( void );

	// re-start service
	bool restart( void )
	{
		return( stop() && start() );
	}

	bool runAsService( void );

	bool evalArgs( int & _argc, char * * _argv );

	inline int & argc( void )
	{
		return( m_argc );
	}

	inline char * * argv( void )
	{
		return( m_argv );
	}


private:
	const QString m_name;
	const QString m_arg;
	const QString m_displayName;
	const QString m_dependencies;
	service_main m_serviceMain;
	bool m_running;
	bool m_quiet;

	int m_argc;
	char * * m_argv;


	typedef void ( * workThreadFunctionPtr )( void * );

	static void serviceMainThread( void * );

#ifdef BUILD_WIN32
	static void WINAPI main( DWORD, char * * );
	static DWORD WINAPI serviceCtrl( DWORD _ctrlcode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext );
	static bool reportStatus( DWORD _state, DWORD _exit_code,
							DWORD _wait_hint );

	// we assume that a process won't contain more than one services
	// therefore we can make these members static
	static systemService *		s_this;
	static SERVICE_STATUS		s_status;
	static SERVICE_STATUS_HANDLE	s_statusHandle;
	static DWORD			s_error;
	static DWORD			s_serviceThread;
#endif

} ;

extern QString __app_name;


#endif

