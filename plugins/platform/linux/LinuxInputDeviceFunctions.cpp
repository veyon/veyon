/*
 * LinuxInputDeviceFunctions.cpp - implementation of LinuxInputDeviceFunctions class
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

#include <QDir>
#include <QFile>

#include "PlatformServiceFunctions.h"
#include "LinuxInputDeviceFunctions.h"
#include "LinuxKeyboardShortcutTrapper.h"


LinuxInputDeviceFunctions::LinuxInputDeviceFunctions() :
	m_isWaylandSession(qEnvironmentVariableIsSet("WAYLAND_DISPLAY"))
{
}

#include <X11/XKBlib.h>

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>


void LinuxInputDeviceFunctions::enableInputDevices()
{
	if( m_inputDevicesDisabled == false )
	{
		return;
	}

	if (m_isWaylandSession)
	{
		enableInputDevicesWayland();
	}
	else
	{
		restoreKeyMapTable();
	}

	m_inputDevicesDisabled = false;
}



void LinuxInputDeviceFunctions::disableInputDevices()
{
	if( m_inputDevicesDisabled )
	{
		return;
	}

	if (m_isWaylandSession)
	{
		disableInputDevicesWayland();
	}
	else
	{
		setEmptyKeyMapTable();
	}

	m_inputDevicesDisabled = true;
}



KeyboardShortcutTrapper* LinuxInputDeviceFunctions::createKeyboardShortcutTrapper( QObject* parent )
{
	return new LinuxKeyboardShortcutTrapper( parent );
}



void LinuxInputDeviceFunctions::setEmptyKeyMapTable()
{
	if( m_origKeyTable )
	{
		XFree( m_origKeyTable );
	}

	auto display = XOpenDisplay( nullptr );
	XDisplayKeycodes( display, &m_keyCodeMin, &m_keyCodeMax );
	m_keyCodeCount = m_keyCodeMax - m_keyCodeMin;

	m_origKeyTable = XGetKeyboardMapping( display, ::KeyCode( m_keyCodeMin ), m_keyCodeCount, &m_keySymsPerKeyCode );

	auto newKeyTable = XGetKeyboardMapping( display, ::KeyCode( m_keyCodeMin ), m_keyCodeCount, &m_keySymsPerKeyCode );

	for( int i = 0; i < m_keyCodeCount * m_keySymsPerKeyCode; i++ )
	{
		newKeyTable[i] = 0;
	}

	XChangeKeyboardMapping( display, m_keyCodeMin, m_keySymsPerKeyCode, newKeyTable, m_keyCodeCount );
	XFlush( display );
	XFree( newKeyTable );
	XCloseDisplay( display );
}



void LinuxInputDeviceFunctions::restoreKeyMapTable()
{
	Display* display = XOpenDisplay( nullptr );

	XChangeKeyboardMapping( display, m_keyCodeMin, m_keySymsPerKeyCode,
							static_cast<::KeySym *>( m_origKeyTable ), m_keyCodeCount );

	XFlush( display );
	XCloseDisplay( display );

	XFree( m_origKeyTable );
	m_origKeyTable = nullptr;
}



// ---------------------------------------------------------------------------
// Wayland input device blocking via evdev EVIOCGRAB
// ---------------------------------------------------------------------------

int LinuxInputDeviceFunctions::openAndGrabInputDevice(const QString& path)
{
	const int fd = ::open( path.toUtf8().constData(), O_RDWR | O_NONBLOCK );
	if( fd < 0 )
	{
		return -1;
	}

	// Try to grab the device exclusively. This prevents Wayland/compositor
	// from receiving events from this device while we hold the grab.
	if( ::ioctl( fd, EVIOCGRAB, reinterpret_cast<void*>( 1L ) ) < 0 )
	{
		::close( fd );
		return -1;
	}

	return fd;
}



void LinuxInputDeviceFunctions::disableInputDevicesWayland()
{
	// Close any previously grabbed devices first
	enableInputDevicesWayland();

	const QDir inputDir( QStringLiteral("/dev/input") );
	if( inputDir.exists() == false )
	{
		vWarning() << "No /dev/input directory found - cannot grab input devices on Wayland";
		return;
	}

	const auto eventFiles = inputDir.entryList( { QStringLiteral("event*") }, QDir::System | QDir::Files );
	for( const auto& eventFile : eventFiles )
	{
		const auto devicePath = inputDir.absoluteFilePath( eventFile );

		// Skip touchpads and other non-keyboard devices? We grab all input
		// devices to ensure complete input blocking for classroom control.
		const int fd = openAndGrabInputDevice( devicePath );
		if( fd >= 0 )
		{
			m_grabbedDeviceFds.append( fd );
			vDebug() << "Grabbed input device:" << devicePath;
		}
	}

	if( m_grabbedDeviceFds.isEmpty() )
	{
		vWarning() << "Could not grab any input device - you might need to run as root";
	}
	else
	{
		vInfo() << "Grabbed" << m_grabbedDeviceFds.size() << "input devices on Wayland";
	}
}



void LinuxInputDeviceFunctions::enableInputDevicesWayland()
{
	for( const auto fd : std::as_const( m_grabbedDeviceFds ) )
	{
		// Release the grab
		if( fd >= 0 )
		{
			::ioctl( fd, EVIOCGRAB, reinterpret_cast<void*>( 0L ) );
			::close( fd );
		}
	}

	m_grabbedDeviceFds.clear();
	vDebug() << "Released all grabbed input devices";
}
