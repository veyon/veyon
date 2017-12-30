/*
 * WindowsInputDeviceFunctions.cpp - implementation of WindowsInputDeviceFunctions class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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
	m_hidServiceActivated( VeyonCore::platform().serviceFunctions().isRunning( m_hidServiceName ) )
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



void WindowsInputDeviceFunctions::restoreHIDService()
{
	if( m_hidServiceActivated )
	{
		VeyonCore::platform().serviceFunctions().start( m_hidServiceName );
	}
}



void WindowsInputDeviceFunctions::stopHIDService()
{
	if( m_hidServiceActivated )
	{
		VeyonCore::platform().serviceFunctions().stop( m_hidServiceName );
	}
}
