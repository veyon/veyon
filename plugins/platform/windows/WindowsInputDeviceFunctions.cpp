/*
 * WindowsInputDeviceFunctions.cpp - implementation of WindowsInputDeviceFunctions class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <windows.h>

#include "ConfigurationManager.h"
#include "PlatformServiceFunctions.h"
#include "WindowsInputDeviceFunctions.h"
#include "WindowsKeyboardShortcutTrapper.h"


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



KeyboardShortcutTrapper* WindowsInputDeviceFunctions::createKeyboardShortcutTrapper( QObject* parent )
{
	return new WindowsKeyboardShortcutTrapper( parent );
}



void WindowsInputDeviceFunctions::checkInterceptionInstallation()
{
	const auto context = interception_create_context();
	if( context )
	{
		// a valid context means the interception driver is installed properly
		// so nothing to do here
		interception_destroy_context( context );
	}
	// try to (re)install interception driver
	else if( installInterception() == false )
	{
		// failed to uninstall it so we can try to install it again on next reboot
		uninstallInterception();
	}
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



bool WindowsInputDeviceFunctions::installInterception()
{
	return interceptionInstaller( QStringLiteral("/install") ) == 0;
}



bool WindowsInputDeviceFunctions::uninstallInterception()
{
	return interceptionInstaller( QStringLiteral("/uninstall") ) == 0;
}



int WindowsInputDeviceFunctions::interceptionInstaller( const QString& argument )
{
	return QProcess::execute( QCoreApplication::applicationDirPath() + QStringLiteral("/interception/install-interception.exe"), { argument } );
}
