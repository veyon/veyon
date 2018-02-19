/*
 * WindowsCoreFunctions.cpp - implementation of WindowsCoreFunctions class
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <QWindow>
#include <qpa/qplatformnativeinterface.h>

#include <shlobj.h>
#include <userenv.h>
#include <wtsapi32.h>

#include "WindowsCoreFunctions.h"
#include "XEventLog.h"


#define SHUTDOWN_FLAGS (EWX_FORCE | EWX_FORCEIFHUNG)
#define SHUTDOWN_REASON SHTDN_REASON_MAJOR_OTHER

static const int screenSaverSettingsCount = 3;
static const UINT screenSaverSettingsGetList[screenSaverSettingsCount] =
{
    SPI_GETLOWPOWERTIMEOUT,
    SPI_GETPOWEROFFTIMEOUT,
    SPI_GETSCREENSAVETIMEOUT
};

static const UINT screenSaverSettingsSetList[screenSaverSettingsCount] =
{
    SPI_SETLOWPOWERTIMEOUT,
    SPI_SETPOWEROFFTIMEOUT,
    SPI_SETSCREENSAVETIMEOUT
};

static int screenSaverSettings[screenSaverSettingsCount];


static DWORD findProcessId( const QString& userName )
{
	DWORD sidLen = SECURITY_MAX_SID_SIZE;
	char userSID[SECURITY_MAX_SID_SIZE];
	wchar_t domainName[MAX_PATH];
	domainName[0] = 0;
	DWORD domainLen = MAX_PATH;
	SID_NAME_USE sidNameUse;

	if( LookupAccountName( nullptr,		// system name
	                       (LPCWSTR) userName.utf16(),
	                       userSID,
	                       &sidLen,
	                       domainName,
	                       &domainLen,
	                       &sidNameUse ) == false )
	{
		qCritical( "Could not look up SID structure" );
		return -1;
	}

	PWTS_PROCESS_INFO processInfo;
	DWORD processCount = 0;

	if( WTSEnumerateProcesses( WTS_CURRENT_SERVER_HANDLE, 0, 1, &processInfo, &processCount ) == false )
	{
		return -1;
	}

	DWORD pid = -1;

	for( DWORD proc = 0; proc < processCount; ++proc )
	{
		if( processInfo[proc].ProcessId > 0 &&
		        EqualSid( processInfo[proc].pUserSid, userSID ) )
		{
			pid = processInfo[proc].ProcessId;
			break;
		}
	}

	WTSFreeMemory( processInfo );

	return pid;
}



WindowsCoreFunctions::WindowsCoreFunctions() :
    m_eventLog( nullptr )
{
}



WindowsCoreFunctions::~WindowsCoreFunctions()
{
	delete m_eventLog;
}



void WindowsCoreFunctions::initNativeLoggingSystem( const QString& appName )
{
	m_eventLog = new CXEventLog( (wchar_t*) appName.utf16() );
}



void WindowsCoreFunctions::writeToNativeLoggingSystem( const QString& message, Logger::LogLevel loglevel )
{
	WORD wtype = -1;

	switch( loglevel )
	{
	case Logger::LogLevelCritical:
	case Logger::LogLevelError: wtype = EVENTLOG_ERROR_TYPE; break;
	case Logger::LogLevelWarning: wtype = EVENTLOG_WARNING_TYPE; break;
		//case LogLevelInfo: wtype = EVENTLOG_INFORMATION_TYPE; break;
	default:
		break;
	}

	if( wtype > 0 )
	{
		m_eventLog->Write( wtype, (wchar_t*) message.utf16() );
	}
}



void WindowsCoreFunctions::reboot()
{
	enablePrivilege( SE_SHUTDOWN_NAME, true );
	ExitWindowsEx( EWX_REBOOT | SHUTDOWN_FLAGS, SHUTDOWN_REASON );
}



void WindowsCoreFunctions::powerDown()
{
	enablePrivilege( SE_SHUTDOWN_NAME, true );
	ExitWindowsEx( EWX_POWEROFF | SHUTDOWN_FLAGS, SHUTDOWN_REASON );
}



static QWindow* windowForWidget( const QWidget* widget )
{
	QWindow* window = widget->windowHandle();
	if( window )
	{
		return window;
	}

	const QWidget* nativeParent = widget->nativeParentWidget();
	if( nativeParent )
	{
		return nativeParent->windowHandle();
	}

	return 0;
}



void WindowsCoreFunctions::raiseWindow( QWidget* widget )
{
	widget->activateWindow();
	widget->raise();

	QWindow* window = windowForWidget( widget );
	if( window )
	{
		QPlatformNativeInterface* interfacep = QGuiApplication::platformNativeInterface();
		auto windowHandle = static_cast<HWND>( interfacep->nativeResourceForWindow( QByteArrayLiteral( "handle" ), window ) );

		SetWindowPos( windowHandle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
		SetWindowPos( windowHandle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
	}
}



void WindowsCoreFunctions::disableScreenSaver()
{
	for( int i = 0; i < screenSaverSettingsCount; ++i )
	{
		SystemParametersInfo( screenSaverSettingsGetList[i], 0, &screenSaverSettings[i], 0 );
		SystemParametersInfo( screenSaverSettingsSetList[i], 0, nullptr, 0 );
	}
}



void WindowsCoreFunctions::restoreScreenSaverSettings()
{
	for( int i = 0; i < screenSaverSettingsCount; ++i )
	{
		SystemParametersInfo( screenSaverSettingsSetList[i], screenSaverSettings[i], nullptr, 0 );
	}
}



QString WindowsCoreFunctions::activeDesktopName()
{
	QString desktopName;

	HDESK desktopHandle = OpenInputDesktop( 0, true, DESKTOP_READOBJECTS );

	wchar_t inputDesktopName[256];
	inputDesktopName[0] = 0;
	if( GetUserObjectInformation( desktopHandle, UOI_NAME, inputDesktopName,
	                              sizeof( inputDesktopName ) / sizeof( wchar_t ), nullptr ) )
	{
		desktopName = QString( QStringLiteral( "winsta0\\%1" ) ).arg( QString::fromWCharArray( inputDesktopName ) );
	}
	CloseDesktop( desktopHandle );

	return desktopName;
}



bool WindowsCoreFunctions::isRunningAsAdmin() const
{
	BOOL runningAsAdmin = false;
	PSID adminGroupSid = nullptr;

	// allocate and initialize a SID of the administrators group.
	SID_IDENTIFIER_AUTHORITY NtAuthority = { SECURITY_NT_AUTHORITY };
	if( AllocateAndInitializeSid(
	            &NtAuthority,
	            2,
	            SECURITY_BUILTIN_DOMAIN_RID,
	            DOMAIN_ALIAS_RID_ADMINS,
	            0, 0, 0, 0, 0, 0,
	            &adminGroupSid ) )
	{
		// determine whether the SID of administrators group is enabled in
		// the primary access token of the process.
		CheckTokenMembership( nullptr, adminGroupSid, &runningAsAdmin );
	}

	if( adminGroupSid )
	{
		FreeSid( adminGroupSid );
	}

	return runningAsAdmin;
}



bool WindowsCoreFunctions::runProgramAsAdmin( const QString& program, const QStringList& parameters )
{
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.lpVerb = L"runas";
	sei.lpFile = (LPWSTR) program.utf16();
	sei.hwnd = GetForegroundWindow();
	sei.lpParameters = (LPWSTR) parameters.join( ' ' ).utf16();
	sei.nShow = SW_NORMAL;

	if( ShellExecuteEx( &sei ) == false )
	{
		return false;
	}

	return true;
}



bool WindowsCoreFunctions::runProgramAsUser( const QString& program,
                                             const QString& username,
                                             const QString& desktop )
{
	enablePrivilege( SE_ASSIGNPRIMARYTOKEN_NAME, true );
	enablePrivilege( SE_INCREASE_QUOTA_NAME, true );

	const auto userProcessHandle = OpenProcess( PROCESS_ALL_ACCESS, false,
	                                            findProcessId( username ) );

	HANDLE userProcessToken = nullptr;
	if( OpenProcessToken( userProcessHandle, MAXIMUM_ALLOWED, &userProcessToken ) == false )
	{
		qCritical() << Q_FUNC_INFO << "OpenProcessToken()" << GetLastError();
		return false;
	}

	LPVOID userEnvironment = nullptr;
	if( CreateEnvironmentBlock( &userEnvironment, userProcessToken, false ) == false )
	{
		qCritical() << Q_FUNC_INFO << "CreateEnvironmentBlock()" << GetLastError();
		return false;
	}

	PWSTR profileDir = nullptr;
	if( SHGetKnownFolderPath( FOLDERID_Profile, 0, userProcessToken, &profileDir ) != S_OK )
	{
		qCritical() << Q_FUNC_INFO << "SHGetKnownFolderPath()" << GetLastError();
		return false;
	}

	if( ImpersonateLoggedOnUser( userProcessToken ) == false )
	{
		qCritical() << Q_FUNC_INFO << "ImpersonateLoggedOnUser()" << GetLastError();
		return false;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory( &si, sizeof( STARTUPINFO ) );
	si.cb = sizeof( STARTUPINFO );
	if( !desktop.isEmpty() )
	{
		si.lpDesktop = (LPWSTR) desktop.utf16();
	}

	HANDLE newToken = nullptr;

	DuplicateTokenEx( userProcessToken, TOKEN_ASSIGN_PRIMARY|TOKEN_ALL_ACCESS, nullptr,
	                  SecurityImpersonation, TokenPrimary, &newToken );

	if( CreateProcessAsUser(
	            newToken,			// client's access token
	            nullptr,			  // file to execute
	            (LPWSTR) program.utf16(),	 // command line
	            nullptr,			  // pointer to process SECURITY_ATTRIBUTES
	            nullptr,			  // pointer to thread SECURITY_ATTRIBUTES
	            false,			 // handles are not inheritable
	            CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS,   // creation flags
	            userEnvironment,			  // pointer to new environment block
	            profileDir,			  // name of current directory
	            &si,			   // pointer to STARTUPINFO structure
	            &pi				// receives information about new process
	            ) == false )
	{
		qCritical() << Q_FUNC_INFO << "CreateProcessAsUser()" << GetLastError();
		return false;
	}

	CoTaskMemFree( profileDir );
	DestroyEnvironmentBlock( userEnvironment );

	CloseHandle( newToken );
	RevertToSelf();
	CloseHandle( userProcessToken );

	CloseHandle( userProcessHandle );

	CloseHandle( pi.hProcess );

	return true;
}



QString WindowsCoreFunctions::genericUrlHandler() const
{
	return QStringLiteral( "explorer" );
}



bool WindowsCoreFunctions::enablePrivilege( LPCWSTR privilegeName, bool enable )
{
	HANDLE token;
	TOKEN_PRIVILEGES tokenPrivileges;
	LUID luid;

	if( !OpenProcessToken( GetCurrentProcess(),
	                       TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, &token ) )
	{
		return false;
	}

	if( !LookupPrivilegeValue( nullptr, privilegeName, &luid ) )
	{
		return false;
	}

	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Luid = luid;
	tokenPrivileges.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

	bool ret = AdjustTokenPrivileges( token, false, &tokenPrivileges, 0, nullptr, nullptr );

	CloseHandle( token );

	return ret;
}
