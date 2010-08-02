/*
 * SystemService.h - convenient class for using app as system-service
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include <italcconfig.h>

#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#include <wtsapi32.h>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

typedef BOOL(WINAPI *PFN_WTSQuerySessionInformation)( HANDLE, DWORD,
					WTS_INFO_CLASS, LPTSTR*, DWORD* );
extern PFN_WTSQuerySessionInformation pfnWTSQuerySessionInformation;

typedef void(WINAPI *PFN_WTSFreeMemory)( PVOID );
extern PFN_WTSFreeMemory pfnWTSFreeMemory;

#endif

#include <QtCore/QString>
#include <QtCore/QThread>


class SystemService
{
public:
	typedef int( * service_main )( SystemService * );

	SystemService( const QString & _service_name,
			const QString & _service_arg = QString::null,
			const QString & _service_display_name = QString::null,
			const QString & _service_dependencies = QString::null,
			service_main _sm = NULL,
			int _argc = 0,
			char * * _argv = NULL );
	//~SystemService( void );

	// install service - will start at next reboot
	bool install( void );

	// unregister service
	bool remove( void );

	// re-install service
	bool reinstall( void )
	{
		return remove() && install();
	}

	// start service if possible
	bool start( void );

	// stop service if possible
	bool stop( void );

	// re-start service
	bool restart( void )
	{
		return stop() && start();
	}

	bool runAsService( void );

	bool evalArgs( int & _argc, char * * _argv );

	inline int & argc( void )
	{
		return m_argc;
	}

	inline char * * argv( void )
	{
		return m_argv;
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

	class workThread : public QThread
	{
	public:
		workThread( workThreadFunctionPtr _ptr, void * _user = NULL ) :
			QThread(),
			m_workThreadFunction( _ptr ),
			m_user( _user )
		{
			start();
		}
		virtual void run( void )
		{
			m_workThreadFunction( m_user );
			deleteLater();
		}
	private:
		workThreadFunctionPtr m_workThreadFunction;
		void * m_user;
	} ;

	static void serviceMainThread( void * );

#ifdef ITALC_BUILD_WIN32
	static void WINAPI main( DWORD, char * * );
	static DWORD WINAPI serviceCtrl( DWORD _ctrlcode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext );
	static bool reportStatus( DWORD _state, DWORD _exit_code,
							DWORD _wait_hint );

	// we assume that a process won't contain more than one services
	// therefore we can make these members static
	static SystemService *		s_this;
	static SERVICE_STATUS		s_status;
	static SERVICE_STATUS_HANDLE	s_statusHandle;
	static DWORD			s_error;
	static DWORD			s_serviceThread;
#endif

} ;

extern QString __app_name;


#endif

