/*
 * WindowsCoreFunctions.cpp - implementation of WindowsCoreFunctions class
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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
#include <QScreen>
#include <QWidget>
#include <QWinEventNotifier>
#include <qpa/qplatformnativeinterface.h>

#include <shlobj.h>
#include <userenv.h>
#include <sddl.h>
#include <tlhelp32.h>

#include "VeyonConfiguration.h"
#include "WindowsCoreFunctions.h"
#include "WindowsPlatformConfiguration.h"
#include "WindowsPlatformPlugin.h"
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



QObject* WindowsCoreFunctions::notifyOnStandardInputReadyRead(const NotifierCallback& callback)
{
	auto notifier = new QWinEventNotifier(GetStdHandle(STD_INPUT_HANDLE));
	QObject::connect(notifier, &QWinEventNotifier::activated,
					 QCoreApplication::instance(),
					 [notifier, callback]() { callback(notifier); });
	return notifier;
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
	SmartSID adminGroupSid;

	// allocate and initialize a SID of the administrators group.
	SID_IDENTIFIER_AUTHORITY NtAuthority = { SECURITY_NT_AUTHORITY };
	if (AllocateAndInitializeSid(
			&NtAuthority,
			2,
			SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS,
			0, 0, 0, 0, 0, 0,
			adminGroupSid.put()))
	{
		// determine whether the SID of administrators group is enabled in
		// the primary access token of the process.
		CheckTokenMembership(nullptr, adminGroupSid.get(), &runningAsAdmin);
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



bool WindowsCoreFunctions::runProgramAsUser(const QString& program,
											const QStringList& parameters,
											const QString& username,
											const QString& desktop,
											const QByteArray& stdInData)
{
	vDebug() << program << parameters << username << desktop;

	const auto baseProcessId = WtsSessionManager::findUserProcessId( username );
	if( baseProcessId == WtsSessionManager::InvalidProcess )
	{
		vCritical() << "could not determine base process ID for user" << username;
		return false;
	}

	return runProgramInSession(program, parameters, {}, baseProcessId, desktop, stdInData).isValid();
}



QString WindowsCoreFunctions::genericUrlHandler() const
{
	return QStringLiteral( "explorer" );
}



QString WindowsCoreFunctions::queryDisplayDeviceName(const QScreen& screen) const
{
	if(screen.name().isEmpty())
	{
		return {};
	}

	const auto screenDeviceName = toConstWCharArray(screen.name());

	const auto fallbackEnumDisplayDevices = [&]() -> QString {
		DISPLAY_DEVICE displayDevice{};
		displayDevice.cb = sizeof(displayDevice);
		if (EnumDisplayDevicesW(screenDeviceName, 0, &displayDevice, 0))
		{
			return QString::fromWCharArray(displayDevice.DeviceString);
		}
		return {};
	};

	UINT32 requiredPaths, requiredModes;
	auto result = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &requiredPaths, &requiredModes);
	if (result != ERROR_SUCCESS || requiredPaths == 0 || requiredModes == 0)
	{
		return fallbackEnumDisplayDevices();
	}

	std::vector<DISPLAYCONFIG_PATH_INFO> paths(requiredPaths);
	std::vector<DISPLAYCONFIG_MODE_INFO> modes(requiredModes);
	if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &requiredPaths, paths.data(),
						   &requiredModes, modes.data(), nullptr) != ERROR_SUCCESS)
	{
		return fallbackEnumDisplayDevices();
	}

	for (const auto& p : paths)
	{
		DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName{};
		sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
		sourceName.header.size = sizeof(sourceName);
		sourceName.header.adapterId = p.sourceInfo.adapterId;
		sourceName.header.id = p.sourceInfo.id;

		if (DisplayConfigGetDeviceInfo(&sourceName.header) != ERROR_SUCCESS)
		{
			continue;
		}

		if (wcscmp(screenDeviceName, sourceName.viewGdiDeviceName) == 0)
		{
			DISPLAYCONFIG_TARGET_DEVICE_NAME name{};
			name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
			name.header.size = sizeof(name);
			name.header.adapterId = p.sourceInfo.adapterId;
			name.header.id = p.targetInfo.id;

			if (DisplayConfigGetDeviceInfo(&name.header) != ERROR_SUCCESS)
			{
				return fallbackEnumDisplayDevices();
			}

			const auto monitorFriendlyDeviceName = QString::fromWCharArray(name.monitorFriendlyDeviceName);
			const auto outputName = [&]() -> QString {
				switch(name.outputTechnology)
				{
				case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HDMI:
					return QStringLiteral("HDMI-%1").arg(name.connectorInstance);
				case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DVI:
					return QStringLiteral("DVI-%1").arg(name.connectorInstance);
				case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED:
					return QStringLiteral("eDP-%1").arg(name.connectorInstance);
				case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EXTERNAL:
					return QStringLiteral("DP-%1").arg(name.connectorInstance);
				case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_LVDS:
					return QStringLiteral("LVDS-%1").arg(name.connectorInstance);
				case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_MIRACAST:
					return QStringLiteral("Miracast-%1").arg(name.connectorInstance);
				case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SVIDEO:
					return QStringLiteral("S-Video-%1").arg(name.connectorInstance);
				case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL:
					return WindowsPlatformPlugin::tr("Internal display") +
							( name.connectorInstance > 1 ?
								  QStringLiteral(" %2").arg(name.connectorInstance) : QString{} );
				default:
					break;
				}
				return {};
			}();
			if(monitorFriendlyDeviceName.isEmpty())
			{
				if(outputName.isEmpty() == false)
				{
					return outputName;
				}
				// use display device string as fallback
			}
			else
			{
				return monitorFriendlyDeviceName +
						( outputName.isEmpty() ? QString {} : QStringLiteral(" [%1]").arg(outputName) );
			}
		}
	}

	return fallbackEnumDisplayDevices();
}



QString WindowsCoreFunctions::getApplicationName(ProcessId processId) const
{
	if (processId == InvalidProcessId)
	{
		return {};
	}

	struct WindowSearchData {
		DWORD pid;
		QString title;
	};

	WindowSearchData data = {DWORD(processId), {}};

	EnumWindows([](HWND window, LPARAM instance) -> WINBOOL CALLBACK {
		WindowSearchData* data = reinterpret_cast<WindowSearchData*>(instance);
		DWORD winPid;
		GetWindowThreadProcessId(window, &winPid);
		if (winPid == data->pid && IsWindowVisible(window))
		{
			const auto len = GetWindowTextLengthW(window);
			if (len > 0)
			{
				std::wstring winTitle(len, L'\0');
				GetWindowTextW(window, &winTitle[0], len + 1);
				data->title = QString::fromStdWString(winTitle);
				return FALSE;
			}
		}
		return TRUE;
	}, reinterpret_cast<LPARAM>(&data));

	if (data.title.isEmpty())
	{
		const auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE)
		{
			return {};
		}

		PROCESSENTRY32W pe;
		pe.dwSize = sizeof(PROCESSENTRY32W);

		if (Process32FirstW(hSnapshot, &pe))
		{
			do {
				if (pe.th32ProcessID == data.pid)
				{
					data.title = QString::fromWCharArray(pe.szExeFile);
					break;
				}
			} while (Process32NextW(hSnapshot, &pe));
		}
		CloseHandle(hSnapshot);
	}

	return data.title;
}



bool WindowsCoreFunctions::enablePrivilege( LPCWSTR privilegeName, bool enable )
{
	SmartToken processToken;
	if (!OpenProcessToken(GetCurrentProcess(),
						  TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, processToken.put()))
	{
		vCritical() << "could not open process token";
		return false;
	}

	LUID luid{};
	if (!LookupPrivilegeValue(nullptr, privilegeName, &luid))
	{
		vCritical() << "could not lookup privilege value";
		return false;
	}

	TOKEN_PRIVILEGES tokenPrivileges{};
	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Luid = luid;
	tokenPrivileges.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

	const auto ret = AdjustTokenPrivileges(processToken.get(), false, &tokenPrivileges, 0, nullptr, nullptr );

	return ret;
}



SmartWCharPtr WindowsCoreFunctions::toWCharArray(const QString& qstring)
{
	auto wcharArray = new wchar_t[qstring.size()+1];
	qstring.toWCharArray( wcharArray );
	wcharArray[qstring.size()] = 0;

	return SmartWCharPtr{wcharArray};
}



const wchar_t* WindowsCoreFunctions::toConstWCharArray( const QString& qstring )
{
	return reinterpret_cast<const wchar_t*>( qstring.utf16() );
}



QString WindowsCoreFunctions::securityIdentifierToString(const SecurityIdentifierBuffer& sidBuffer)
{
	SmartStringSID stringSid;
	if (ConvertSidToStringSidW(reinterpret_cast<PSID>(const_cast<std::byte *>(sidBuffer.data())), stringSid.put()))
	{
		const auto sidString = QString::fromWCharArray(stringSid.get());
		return sidString;
	}

	vCritical() << "Could not convert SID to string:" << GetLastError();

	return {};
}



bool WindowsCoreFunctions::stringToSecurityIdentifier(const QString& sidString, WindowsCoreFunctions::SecurityIdentifierBuffer& sidBuffer)
{
	memset(sidBuffer.data(), 0, sidBuffer.size());

	SmartSID sid;
	if (ConvertStringSidToSid(toConstWCharArray(sidString), sid.put()) && sid)
	{
		const auto sidLength = GetLengthSid(sid.get());
		if (sidLength <= sidBuffer.size())
		{
			memcpy(sidBuffer.data(), sid.get(), sidLength);
		}
		return true;
	}

	vCritical() << "Could not convert SID string to SID:" << GetLastError();

	return false;
}



SmartHandle WindowsCoreFunctions::runProgramInSession(const QString& program,
													  const QStringList& parameters,
													  const QStringList& extraEnvironment,
													  DWORD baseProcessId,
													  const QString& desktop,
													  const QByteArray& stdInData)
{
	vDebug() << program << parameters << extraEnvironment << baseProcessId;

	enablePrivilege( SE_ASSIGNPRIMARYTOKEN_NAME, true );
	enablePrivilege( SE_INCREASE_QUOTA_NAME, true );
	enablePrivilege( SE_TCB_NAME, true );

	SmartHandle userProcessHandle{OpenProcess(PROCESS_ALL_ACCESS, false, baseProcessId)};
	if (userProcessHandle.isInvalid())
	{
		vCritical() << "OpenProcess()" << GetLastError();
		return {};
	}

	SmartToken userProcessToken;
	if (OpenProcessToken(userProcessHandle.get(), MAXIMUM_ALLOWED, userProcessToken.put()) == false)
	{
		vCritical() << "OpenProcessToken()" << GetLastError();
		return {};
	}

	SmartEnvBlockPtr userEnvironment;
	if (CreateEnvironmentBlock(userEnvironment.put(), userProcessToken.get(), false) == false)
	{
		vCritical() << "CreateEnvironmentBlock()" << GetLastError();
		return {};
	}

	SmartCoTaskMemPtr<wchar_t> profileDir;
	if (SHGetKnownFolderPath(FOLDERID_Profile, 0, userProcessToken.get(), profileDir.put()) != S_OK)
	{
		vCritical() << "SHGetKnownFolderPath()" << GetLastError();
		return {};
	}

	if (ImpersonateLoggedOnUser(userProcessToken.get()) == false) // Flawfinder: ignore
	{
		vCritical() << "ImpersonateLoggedOnUser()" << GetLastError();
		return {};
	}

	const auto revertToSelfGuard = qScopeGuard(RevertToSelf);

	auto desktopWide = toWCharArray(desktop);

	if (desktop.isEmpty())
	{
		desktopWide = toWCharArray( QStringLiteral("Winsta0\\Default") );
	}

	STARTUPINFO si{};
	si.cb = sizeof(STARTUPINFO);
	si.lpDesktop = desktopWide.get();

	const SmartWCharPtr fullEnvironment{appendToEnvironmentBlock(reinterpret_cast<const wchar_t *>(userEnvironment.get()), extraEnvironment)};

	SmartToken newToken;
	if (DuplicateTokenEx(userProcessToken.get(), TOKEN_ASSIGN_PRIMARY|TOKEN_ALL_ACCESS, nullptr,
						 SecurityImpersonation, TokenPrimary, newToken.put()) == false ||
		newToken.isInvalid())
	{
		vCritical() << "DuplicateTokenEx()" << GetLastError();
		return {};
	}

	SmartFileHandle stdinRead;
	SmartFileHandle stdinWrite;

	if (stdInData.isEmpty() == false)
	{
		SECURITY_ATTRIBUTES sa{};
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = nullptr;
		sa.bInheritHandle = true;

		if (CreatePipe(stdinRead.put(), stdinWrite.put(), &sa, 0) == false)
		{
			vCritical() << "could not create stdin pipe";
			return {};
		}

		if (SetHandleInformation(stdinWrite.get(), HANDLE_FLAG_INHERIT, 0) == false)
		{
			return {};
		}

		si.dwFlags |= STARTF_USESTDHANDLES;
		si.hStdInput = stdinRead.get();
		si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	}

	const auto commandLine = toWCharArray(QStringLiteral("\"%1\" %2").arg(program, parameters.join(QLatin1Char(' '))));

	PROCESS_INFORMATION pi{};
	if (CreateProcessAsUser(
			newToken.get(),			// client's access token
			nullptr,			  // file to execute
			commandLine.get(),	 // command line
			nullptr,			  // pointer to process SECURITY_ATTRIBUTES
			nullptr,			  // pointer to thread SECURITY_ATTRIBUTES
			true,			 // handles are not inheritable
			CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS,   // creation flags
			fullEnvironment ? fullEnvironment.get() : userEnvironment.get(),			  // pointer to new environment block
			profileDir.get(),			  // name of current directory
			&si,			   // pointer to STARTUPINFO structure
			&pi				// receives information about new process
			) == false)
	{
		vCritical() << "CreateProcessAsUser()" << GetLastError();
		return {};
	}

	SmartHandle threadHandle{pi.hThread};
	SmartHandle processHandle{pi.hProcess};

	if (stdInData.isEmpty() == false)
	{
		DWORD written = 0;

		if (WriteFile(stdinWrite.get(), stdInData.constData(), DWORD(stdInData.size()), &written, nullptr) == false ||
			written != stdInData.size())
		{
			vCritical() << "failed to write stdin data";
		}
	}

	return processHandle;
}



QStringList WindowsCoreFunctions::queryProcessEnvironmentVariables(DWORD processId)
{
	SmartHandle processHandle{OpenProcess(PROCESS_QUERY_INFORMATION, false, processId)};
	if (processHandle.isInvalid())
	{
		vCritical() << "OpenProcess()" << GetLastError();
		return {};
	}

	SmartToken processToken;
	if (OpenProcessToken(processHandle.get(), MAXIMUM_ALLOWED, processToken.put()) == false)
	{
		vCritical() << "OpenProcessToken()" << GetLastError();
		return {};
	}

	SmartEnvBlockPtr envBlock;
	if (CreateEnvironmentBlock(envBlock.put(), processToken.get(), false) == false ||
		envBlock.isInvalid())
	{
		vCritical() << "CreateEnvironmentBlock()" << GetLastError();
		return {};
	}

	const auto env = reinterpret_cast<const wchar_t *>(envBlock.get());
	size_t envPos = 0;
	size_t envCurVarStart = 0;

	QStringList envVars;
	while (envPos < MaximumEnvironmentBlockSize-1 && !(env[envPos] == 0 && env[envPos+1] == 0))
	{
		if (env[envPos] == 0)
		{
			envVars.append(QString::fromWCharArray(env + envCurVarStart));
			envCurVarStart = envPos+1;
		}
		++envPos;
	}

	return envVars;
}



bool WindowsCoreFunctions::terminateProcess( ProcessId processId, DWORD timeout )
{
	if( processId != WtsSessionManager::InvalidProcess )
	{
		SmartHandle processHandle{OpenProcess(PROCESS_TERMINATE, false, processId)};
		if( processHandle )
		{
			const auto result = TerminateProcess(processHandle.get(), 0);
			WaitForSingleObject(processHandle.get(), timeout);

			return result;
		}

		vCritical() << "could not open process with ID" << processId;
	}

	return false;
}



wchar_t* WindowsCoreFunctions::appendToEnvironmentBlock( const wchar_t* env, const QStringList& strings )
{
	if (env == nullptr)
	{
		return nullptr;
	}

	static constexpr auto MaximumExtraStringsLength = 1024*1024;

	size_t envPos = 0;
	while (envPos < MaximumEnvironmentBlockSize-1 && !(env[envPos] == 0 && env[envPos+1] == 0))
	{
		++envPos;
	}

	++envPos;

	if (envPos >= MaximumEnvironmentBlockSize-1)
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
