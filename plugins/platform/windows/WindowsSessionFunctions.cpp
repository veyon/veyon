/*
 * WindowsSessionFunctions.cpp - implementation of WindowsSessionFunctions class
 *
 * Copyright (c) 2020-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <wtsapi32.h>

#include <QCoreApplication>
#include <QHostInfo>
#include <QSettings>

#include "WindowsCoreFunctions.h"
#include "PlatformSessionManager.h"
#include "VeyonConfiguration.h"
#include "WindowsPlatformConfiguration.h"
#include "WindowsSessionFunctions.h"
#include "WtsSessionManager.h"


WindowsSessionFunctions::WindowsSessionFunctions()
{
	if (VeyonCore::component() == VeyonCore::Component::Server)
	{
		QObject::connect (VeyonCore::instance(), &VeyonCore::initialized,
						  VeyonCore::instance(), [this]() { initDesktopWindowsRectifier(); });
	}
}



WindowsSessionFunctions::SessionId WindowsSessionFunctions::currentSessionId()
{
	const auto currentSession = WtsSessionManager::currentSession();

	if( currentSession == WtsSessionManager::activeConsoleSession() )
	{
		return DefaultSessionId;
	}

	return PlatformSessionManager::resolveSessionId( QString::number(currentSession) );
}



WindowsSessionFunctions::SessionUptime WindowsSessionFunctions::currentSessionUptime() const
{
	return WtsSessionManager::querySessionInformation(WtsSessionManager::currentSession(),
													  WtsSessionManager::SessionInfo::SessionUptime).toInt();
}



QString WindowsSessionFunctions::currentSessionClientAddress() const
{
	return WtsSessionManager::querySessionInformation(WtsSessionManager::currentSession(),
													  WtsSessionManager::SessionInfo::ClientAddress);
}



QString WindowsSessionFunctions::currentSessionClientName() const
{
	return WtsSessionManager::querySessionInformation(WtsSessionManager::currentSession(),
													  WtsSessionManager::SessionInfo::ClientName);
}



QString WindowsSessionFunctions::currentSessionHostName() const
{
	return QHostInfo::localHostName();
}



QString WindowsSessionFunctions::currentSessionType() const
{
	if(WtsSessionManager::currentSession() == WtsSessionManager::activeConsoleSession() )
	{
		return QStringLiteral("console");
	}

	return QStringLiteral("rdp");
}



bool WindowsSessionFunctions::currentSessionIsRemote() const
{
	return WtsSessionManager::isRemote( WtsSessionManager::currentSession() );
}



bool WindowsSessionFunctions::currentSessionHasUser() const
{
	return WtsSessionManager::querySessionInformation( WtsSessionManager::currentSession(),
													   WtsSessionManager::SessionInfo::UserName ).isEmpty() == false;
}



PlatformSessionFunctions::EnvironmentVariables WindowsSessionFunctions::currentSessionEnvironmentVariables() const
{
	const auto processId = WtsSessionManager::findProcessId(QStringLiteral("explorer.exe"), WtsSessionManager::currentSession());
	const auto envStrings = WindowsCoreFunctions::queryProcessEnvironmentVariables(processId);

	EnvironmentVariables environmentVariables;
	for (const auto& envString : envStrings)
	{
		const auto envStringParts = envString.split(QLatin1Char('='));
		if (envStringParts.size() >= 2)
		{
			environmentVariables[envStringParts.at(0)] = envStringParts.mid(1).join(QLatin1Char('='));
		}
	}

	return environmentVariables;
}



QVariant WindowsSessionFunctions::querySettingsValueInCurrentSession(const QString& key) const
{
	if (key.startsWith(QLatin1String("HKEY")))
	{
		HANDLE userToken = nullptr;
		const auto sessionId = WtsSessionManager::currentSession();
		if (WTSQueryUserToken(sessionId, &userToken) == false)
		{
			vCritical() << "could not query user token for session" << sessionId;
			return {};
		}

		auto keyParts = key.split(QLatin1Char('\\'));
		if (keyParts.constFirst() == QStringLiteral("HKEY_CURRENT_USER"))
		{
			keyParts[0] = WtsSessionManager::queryUserSid(sessionId);
			keyParts.prepend(QStringLiteral("HKEY_USERS"));
		}

		if (ImpersonateLoggedOnUser(userToken) == false)
		{
			vCritical() << "could not impersonate session user";
			return {};
		}

		const auto value = QSettings(keyParts.mid(0, keyParts.length()-1).join(QLatin1Char('\\')), QSettings::NativeFormat)
						   .value(keyParts.constLast());

		RevertToSelf();
		CloseHandle(userToken);

		return value;
	}

	return QSettings(QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName()).value(key);
}


void WindowsSessionFunctions::initDesktopWindowsRectifier()
{
	WindowsPlatformConfiguration config(&VeyonCore::config());

	m_rectifyInterferingWindows = config.rectifyInterferingWindows();
	m_terminateInterferingProcesses = config.terminateInterferingProcesses();

	if (m_rectifyInterferingWindows || m_terminateInterferingProcesses)
	{
		QObject::connect (&m_desktopWindowsInspectionTimer, &QTimer::timeout, &m_desktopWindowsInspectionTimer, [this]() {
			inspectDesktopWindows();
		});
		m_desktopWindowsInspectionTimer.start(DesktopWindowsInspectionInterval);
	}
}



void WindowsSessionFunctions::inspectDesktopWindows()
{
	EnumWindows([](HWND window, LPARAM instance) -> WINBOOL {
		const auto _this = reinterpret_cast<WindowsSessionFunctions *>(instance);
		return _this->inspectDesktopWindow(window);
	}, LPARAM(this));
}



WINBOOL WindowsSessionFunctions::inspectDesktopWindow(HWND window)
{
	const auto windowStyle = GetWindowLong(window, GWL_EXSTYLE);
	if (windowStyle & (WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED)) {
		COLORREF crKey;
		BYTE alpha;
		DWORD flags;
		if (GetLayeredWindowAttributes(window, &crKey, &alpha, &flags) && (flags & LWA_COLORKEY))
		{
			if (m_rectifyInterferingWindows)
			{
				vDebug() << "rectifying window" << window << flags << crKey << alpha;
				SetLayeredWindowAttributes(window, 0, 255, LWA_COLORKEY | LWA_ALPHA);
				SetWindowLong(window, GWL_EXSTYLE,
							  windowStyle & ~(WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST));
				RedrawWindow(window, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
				ShowWindow(window, SW_HIDE);
			}

			if (m_terminateInterferingProcesses)
			{
				vDebug() << "terminating process of window" << window << flags << crKey << alpha;
				DWORD processId = 0;
				if (GetWindowThreadProcessId(window, &processId))
				{
					const auto processHandle = OpenProcess(PROCESS_TERMINATE, 0, processId);
					if (processHandle)
					{
						TerminateProcess(processHandle, 0);
						CloseHandle(processHandle);
					}
					else
					{
						PostMessage(window, WM_QUIT, 0, 0);
					}
				}
			}
		}
	}
	return TRUE;
}
