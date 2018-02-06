/*
 * WindowsInputDeviceFunctions.cpp - implementation of WindowsInputDeviceFunctions class
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

#include <windows.h>

#include "PlatformServiceFunctions.h"
#include "WindowsInputDeviceFunctions.h"


static int interception_is_any( InterceptionDevice device )
{
	Q_UNUSED(device)

	return true;
}



WindowsInputDeviceFunctions::WindowsInputDeviceFunctions() :
    m_inputDevicesDisabled( false ),
    m_interceptionContext( nullptr ),
    m_hidServiceName( QStringLiteral("hidserv") ),
    m_hidServiceStatusInitialized( false ),
    m_hidServiceActivated( false )
{
}



WindowsInputDeviceFunctions::~WindowsInputDeviceFunctions()
{
	if( m_inputDevicesDisabled )
	{
		enableInputDevices();
	}
}



void WindowsInputDeviceFunctions::enableInputDevices()
{
	if( m_inputDevicesDisabled )
	{
		disableInterception();
		restoreHIDService();

		m_inputDevicesDisabled = false;
	}
}



void WindowsInputDeviceFunctions::disableInputDevices()
{
	if( m_inputDevicesDisabled == false )
	{
		enableInterception();
		stopHIDService();

		m_inputDevicesDisabled = true;
	}
}



bool WindowsInputDeviceFunctions::configureSoftwareSAS(bool enabled)
{
	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if( RegCreateKeyEx( HKEY_LOCAL_MACHINE,
						L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
						0, REG_NONE, REG_OPTION_NON_VOLATILE,
						KEY_READ, NULL, &hkLocal, &dw ) != ERROR_SUCCESS)
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
	RegSetValueEx( hkLocalKey, L"SoftwareSASGeneration", 0, REG_DWORD, (LPBYTE) &pref, sizeof(pref) );
	RegCloseKey( hkLocalKey );
	RegCloseKey( hkLocal );

	return true;
}



void WindowsInputDeviceFunctions::enableInterception()
{
	m_interceptionContext = interception_create_context();

	interception_set_filter( m_interceptionContext,
							 interception_is_any,
							 INTERCEPTION_FILTER_KEY_ALL | INTERCEPTION_FILTER_MOUSE_ALL );
}



void WindowsInputDeviceFunctions::disableInterception()
{
	if( m_interceptionContext )
	{
		interception_destroy_context( m_interceptionContext );

		m_interceptionContext = nullptr;
	}
}



void WindowsInputDeviceFunctions::initHIDServiceStatus()
{
	if( m_hidServiceStatusInitialized == false )
	{
		m_hidServiceActivated = VeyonCore::platform().serviceFunctions().isRunning( m_hidServiceName );
		m_hidServiceStatusInitialized = true;
	}
}



void WindowsInputDeviceFunctions::restoreHIDService()
{
	if( m_hidServiceActivated )
	{
		VeyonCore::platform().serviceFunctions().start( m_hidServiceName );
	}
}



void WindowsInputDeviceFunctions::stopHIDService()
{
	initHIDServiceStatus();

	if( m_hidServiceActivated )
	{
		VeyonCore::platform().serviceFunctions().stop( m_hidServiceName );
	}
}
