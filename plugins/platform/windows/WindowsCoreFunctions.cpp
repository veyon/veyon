/*
 * WindowsCoreFunctions.cpp - implementation of WindowsCoreFunctions class
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#include <QGuiApplication>
#include <QWidget>
#include <qpa/qplatformnativeinterface.h>

#include <shlobj.h>
#include <userenv.h>

#include "VeyonConfiguration.h"
#include "WindowsCoreFunctions.h"
#include "WindowsPlatformConfiguration.h"
#include "WtsSessionManager.h"
#include "XEventLog.h"



static bool configureSoftwareSAS( bool enabled )
{
	HKEY hkLocal;
	HKEY hkLocalKey;
	DWORD dw;
	if( RegCreateKeyEx( HKEY_LOCAL_MACHINE,
						L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
						0, REG_NONE, REG_OPTION_NON_VOLATILE,
						KEY_READ, nullptr, &hkLocal, &dw ) != ERROR_SUCCESS)
	{
		return false;
	}

	if( RegOpenKeyEx( hkLocal,
					  L"System",
					  0, KEY_WRITE | KEY_READ,
					  &hkLocalKey ) != ERROR_SUCCESS )
	{
		RegCloseKey( hkLocal );
		return false;
	}

	LONG pref = enabled ? 1 : 0;
	RegSetValueEx( hkLocalKey, L"SoftwareSASGeneration", 0, REG_DWORD, reinterpret_cast<LPBYTE>( &pref ), sizeof(pref) );
	RegCloseKey( hkLocalKey );
	RegCloseKey( hkLocal );

	return true;
}



WindowsCoreFunctions::~WindowsCoreFunctions()
{
	delete m_eventLog;
}



bool WindowsCoreFunctions::applyConfiguration()
{
	WindowsPlatformConfiguration config( &VeyonCore::config() );

	if( configureSoftwareSAS( config.isSoftwareSASEnabled() ) == false )
	{
		vCritical() << WindowsPlatformConfiguration::tr( "Could not change the setting for SAS generation by software. "
														 "Sending Ctrl+Alt+Del via remote control will not work!" );
		return false;
	}

	return true;
}



void WindowsCoreFunctions::initNativeLoggingSystem( const QString& appName )
{
	SetConsoleOutputCP( CP_UTF8 );
	setvbuf( stdout, nullptr, _IOFBF, ConsoleOutputBufferSize );
	setvbuf( stderr, nullptr, _IOFBF, ConsoleOutputBufferSize );

	m_eventLog = new CXEventLog( toConstWCharArray( appName ) );
}



void WindowsCoreFunctions::writeToNativeLoggingSystem( const QString& message, Logger::LogLevel loglevel )
{
	int messageType = -1;

	switch( loglevel )
	{
	case Logger::LogLevel::Critical:
	case Logger::LogLevel::Error: messageType = EVENTLOG_ERROR_TYPE; break;
	case Logger::LogLevel::Warning: messageType = EVENTLOG_WARNING_TYPE; break;
	default:
		break;
	}

	if( messageType > 0 && m_eventLog )
	{
		m_eventLog->Write( static_cast<WORD>( messageType ), toConstWCharArray( message ) );
	}
}



void WindowsCoreFunctions::reboot()
{
	enablePrivilege( SE_SHUTDOWN_NAME, true );
	InitiateShutdown( nullptr, nullptr, 0, ShutdownFlags | SHUTDOWN_RESTART, ShutdownReason );
}



void WindowsCoreFunctions::powerDown( bool installUpdates )
{
	enablePrivilege( SE_SHUTDOWN_NAME, true );
	InitiateShutdown( nullptr, nullptr, 0,
					  ShutdownFlags | SHUTDOWN_POWEROFF | ( installUpdates ? SHUTDOWN_INSTALL_UPDATES : 0 ),
					  ShutdownReason );
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

	return nullptr;
}



void WindowsCoreFunctions::raiseWindow( QWidget* widget, bool stayOnTop )
{
	widget->activateWindow();
	widget->raise();

	auto window = windowForWidget( widget );
	if( window )
	{
		auto windowHandle = HWND( QGuiApplication::platformNativeInterface()->
								  nativeResourceForWindow( QByteArrayLiteral( "handle" ), window ) );

		SetWindowPos( windowHandle, stayOnTop ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
	}
}



void WindowsCoreFunctions::disableScreenSaver()
{
	for( size_t i = 0; i < ScreenSaverSettingsCount; ++i )
	{
		SystemParametersInfo( ScreenSaverSettingsGetList.at(i), 0, &m_screenSaverSettings.at(i), 0 );
		SystemParametersInfo( ScreenSaverSettingsSetList.at(i), 0, nullptr, 0 );
	}

	SetThreadExecutionState( ES_CONTINUOUS | ES_DISPLAY_REQUIRED );
}



void WindowsCoreFunctions::restoreScreenSaverSettings()
{
	for( size_t i = 0; i < ScreenSaverSettingsCount; ++i )
	{
		SystemParametersInfo( ScreenSaverSettingsSetList.at(i), m_screenSaverSettings.at(i), nullptr, 0 );
	}

	SetThreadExecutionState( ES_CONTINUOUS );
}



void WindowsCoreFunctions::setSystemUiState( bool enabled )
{
	WindowsPlatformConfiguration config( &VeyonCore::config() );

	if( config.hideTaskbarForScreenLock() )
	{
		setTaskbarState( enabled );
	}

	if( config.hideStartMenuForScreenLock() )
	{
		setStartMenuState( enabled );
	}

	if( config.hideDesktopForScreenLock() )
	{
		setDesktopState( enabled );
	}
}



QString WindowsCoreFunctions::activeDesktopName()
{
	QString desktopName;

	auto desktopHandle = GetThreadDesktop( GetCurrentThreadId() );

	std::array<wchar_t, MAX_PATH> inputDesktopName{};
	if( GetUserObjectInformation( desktopHandle, UOI_NAME, inputDesktopName.data(), inputDesktopName.size(), nullptr ) )
	{
		desktopName = QString( QStringLiteral( "winsta0\\%1" ) ).arg( QString::fromWCharArray( inputDesktopName.data() ) );
	}

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
	const auto parametersJoined = parameters.join( QLatin1Char(' ') );

	SHELLEXECUTEINFO sei{};
	sei.cbSize = sizeof(sei);
	sei.lpVerb = L"runas";
	sei.lpFile = toConstWCharArray( program );
	sei.hwnd = GetForegroundWindow();
	sei.lpParameters = toConstWCharArray( parametersJoined );
	sei.nShow = SW_NORMAL;

	if( ShellExecuteEx( &sei ) == false )
	{
		return false;
	}

	return true;
}



bool WindowsCoreFunctions::runProgramAsUser( const QString& program,
											 const QStringList& parameters,
											 const QString& username,
											 const QString& desktop )
{
	vDebug() << program << parameters << username << desktop;

	const auto baseProcessId = WtsSessionManager::findUserProcessId( username );
	if( baseProcessId == WtsSessionManager::InvalidProcess )
	{
		vCritical() << "could not determine base process ID for user" << username;
		return false;
	}

	auto processHandle = runProgramInSession( program, parameters, {}, baseProcessId, desktop );
	if( processHandle )
	{
		CloseHandle( processHandle );
		return true;
	}

	return false;
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
		vCritical() << "could not open process token";
		return false;
	}

	if( !LookupPrivilegeValue( nullptr, privilegeName, &luid ) )
	{
		vCritical() << "could lookup privilege value";
		return false;
	}

	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Luid = luid;
	tokenPrivileges.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

	const auto ret = AdjustTokenPrivileges( token, false, &tokenPrivileges, 0, nullptr, nullptr );

	CloseHandle( token );

	return ret;
}



QSharedPointer<wchar_t> WindowsCoreFunctions::toWCharArray( const QString& qstring )
{
	auto wcharArray = new wchar_t[qstring.size()+1];
	qstring.toWCharArray( wcharArray );
	wcharArray[qstring.size()] = 0;

	return { wcharArray, []( const wchar_t* buffer ) { delete[] buffer; } };
}



const wchar_t* WindowsCoreFunctions::toConstWCharArray( const QString& qstring )
{
	return reinterpret_cast<const wchar_t*>( qstring.utf16() );
}



HANDLE WindowsCoreFunctions::runProgramInSession( const QString& program,
												  const QStringList& parameters,
												  const QStringList& extraEnvironment,
												  DWORD baseProcessId,
												  const QString& desktop )
{
	vDebug() << program << parameters << extraEnvironment << baseProcessId;

	enablePrivilege( SE_ASSIGNPRIMARYTOKEN_NAME, true );
	enablePrivilege( SE_INCREASE_QUOTA_NAME, true );
	enablePrivilege( SE_TCB_NAME, true );

	const auto userProcessHandle = OpenProcess( PROCESS_ALL_ACCESS, false, baseProcessId );
	if( userProcessHandle == nullptr )
	{
		vCritical() << "OpenProcess()" << GetLastError();
		return nullptr;
	}

	HANDLE userProcessToken = nullptr;
	if( OpenProcessToken( userProcessHandle, MAXIMUM_ALLOWED, &userProcessToken ) == false )
	{
		vCritical() << "OpenProcessToken()" << GetLastError();
		CloseHandle( userProcessHandle );
		return nullptr;
	}

	LPVOID userEnvironment = nullptr;
	if( CreateEnvironmentBlock( &userEnvironment, userProcessToken, false ) == false )
	{
		vCritical() << "CreateEnvironmentBlock()" << GetLastError();
		CloseHandle( userProcessHandle );
		CloseHandle( userProcessToken );
		return nullptr;
	}

	PWSTR profileDir = nullptr;
	if( SHGetKnownFolderPath( FOLDERID_Profile, 0, userProcessToken, &profileDir ) != S_OK )
	{
		vCritical() << "SHGetKnownFolderPath()" << GetLastError();
		DestroyEnvironmentBlock( userEnvironment );
		CloseHandle( userProcessHandle );
		CloseHandle( userProcessToken );
		return nullptr;
	}

	if( ImpersonateLoggedOnUser( userProcessToken ) == false )
	{
		vCritical() << "ImpersonateLoggedOnUser()" << GetLastError();
		CoTaskMemFree( profileDir );
		DestroyEnvironmentBlock( userEnvironment );
		CloseHandle( userProcessHandle );
		CloseHandle( userProcessToken );
		return nullptr;
	}

	auto desktopWide = toWCharArray( desktop );

	if( desktop.isEmpty() )
	{
		desktopWide = toWCharArray( QStringLiteral("Winsta0\\Default") );
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory( &si, sizeof( STARTUPINFO ) );
	si.cb = sizeof( STARTUPINFO );
	si.lpDesktop = desktopWide.data();

	auto fullEnvironment = appendToEnvironmentBlock( reinterpret_cast<const wchar_t *>( userEnvironment ), extraEnvironment );

	HANDLE newToken = nullptr;

	DuplicateTokenEx( userProcessToken, TOKEN_ASSIGN_PRIMARY|TOKEN_ALL_ACCESS, nullptr,
					  SecurityImpersonation, TokenPrimary, &newToken );

	auto commandLine = toWCharArray( QStringLiteral("\"%1\" %2").arg( program, parameters.join( QLatin1Char(' ') ) ) );

	auto createProcessResult = CreateProcessAsUser(
				newToken,			// client's access token
				nullptr,			  // file to execute
				commandLine.data(),	 // command line
				nullptr,			  // pointer to process SECURITY_ATTRIBUTES
				nullptr,			  // pointer to thread SECURITY_ATTRIBUTES
				false,			 // handles are not inheritable
				CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS,   // creation flags
				fullEnvironment ? fullEnvironment : userEnvironment,			  // pointer to new environment block
				profileDir,			  // name of current directory
				&si,			   // pointer to STARTUPINFO structure
				&pi				// receives information about new process
				);

	if( createProcessResult == false )
	{
		vCritical() << "CreateProcessAsUser()" << GetLastError();
	}

	delete[] fullEnvironment;

	CoTaskMemFree( profileDir );
	DestroyEnvironmentBlock( userEnvironment );

	CloseHandle( newToken );
	RevertToSelf();

	CloseHandle( userProcessToken );
	CloseHandle( userProcessHandle );

	if( createProcessResult )
	{
		return pi.hProcess;
	}

	return nullptr;
}



bool WindowsCoreFunctions::terminateProcess( ProcessId processId, DWORD timeout )
{
	if( processId != WtsSessionManager::InvalidProcess )
	{
		const auto processHandle = OpenProcess( PROCESS_TERMINATE, false, processId );
		if( processHandle )
		{
			const auto result = TerminateProcess( processHandle, 0 );
			WaitForSingleObject( processHandle, timeout );
			CloseHandle( processHandle );

			return result;
		}

		vCritical() << "could not open process with ID" << processId;
	}

	return false;
}



wchar_t* WindowsCoreFunctions::appendToEnvironmentBlock( const wchar_t* env, const QStringList& strings )
{
	static constexpr auto MaximumEnvironmentSize = 1024*1024;
	static constexpr auto MaximumExtraStringsLength = 1024*1024;

	size_t envPos = 0;
	while( envPos < MaximumEnvironmentSize-1 && !(env[envPos] == 0 && env[envPos+1] == 0) )
	{
		++envPos;
	}

	++envPos;

	if( envPos >= MaximumEnvironmentSize-1 )
	{
		return nullptr;
	}

	const auto stringsTotalLength = size_t( strings.join( QLatin1Char(' ') ).size() );
	if( stringsTotalLength >= MaximumExtraStringsLength )
	{
		return nullptr;
	}

	auto newEnv = new wchar_t[envPos + stringsTotalLength + 2];
	memcpy( newEnv, env, envPos*2 ); // Flawfinder: ignore

	for( const auto& string : strings )
	{
		const auto stringLength = size_t(string.size());
		memcpy( &newEnv[envPos], toConstWCharArray( string ), stringLength * sizeof(wchar_t) ); // Flawfinder: ignore
		envPos += stringLength;
		newEnv[envPos] = 0;
		++envPos;
	}

	newEnv[envPos] = 0;

	return newEnv;
}



void WindowsCoreFunctions::setTaskbarState( bool enabled )
{
	auto taskbar = FindWindow( L"Shell_TrayWnd", nullptr );
	auto startButton = FindWindow( L"Start", L"Start" );
	if( startButton == nullptr )
	{
		// Win 7
		startButton = FindWindow( L"Button", L"Start" );
	}

	ShowWindow( taskbar, enabled ? SW_SHOW : SW_HIDE );
	ShowWindow( startButton, enabled ? SW_SHOW : SW_HIDE );

	EnableWindow( taskbar, enabled );
	EnableWindow( startButton, enabled );
}



void WindowsCoreFunctions::setStartMenuState( bool enabled )
{
	auto startMenu = FindWindow( L"Windows.UI.Core.CoreWindow", L"Start" );
	if( startMenu )
	{
		ShowWindow( startMenu, enabled ? SW_SHOWDEFAULT : SW_HIDE );
	}
	else
	{
		startMenu = FindWindow( L"DV2ControlHost", nullptr );
		if( enabled == false )
		{
			ShowWindow( startMenu, SW_HIDE );
		}
		EnableWindow( startMenu, enabled );
	}
}



void WindowsCoreFunctions::setDesktopState( bool enabled )
{
	ShowWindow( FindWindow( L"Progman", nullptr ), enabled ? SW_SHOW : SW_HIDE );
}
