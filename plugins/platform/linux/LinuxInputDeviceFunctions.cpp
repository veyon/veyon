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

#include "PlatformServiceFunctions.h"
#include "InputBlockHelper.h"
#include "LinuxInputDeviceFunctions.h"
#include "LinuxKeyboardShortcutTrapper.h"

#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>


LinuxInputDeviceFunctions::LinuxInputDeviceFunctions() :
	m_isWaylandSession(qEnvironmentVariableIsSet("WAYLAND_DISPLAY"))
{
	if( m_isWaylandSession )
	{
		m_inputBlockHelper = new InputBlockHelper;
	}
}

LinuxInputDeviceFunctions::~LinuxInputDeviceFunctions()
{
	ungrabX11InputDevices();
	delete m_inputBlockHelper;
}


void LinuxInputDeviceFunctions::enableInputDevices()
{
	if( m_isWaylandSession )
	{
		// Always send unblock — the daemon tracks state, unblock is idempotent
		enableInputDevicesWayland();
		m_inputDevicesDisabled = false;
		return;
	}

	if( m_inputDevicesDisabled == false )
		return;

	ungrabX11InputDevices();
	m_inputDevicesDisabled = false;
}



void LinuxInputDeviceFunctions::disableInputDevices()
{
	if( m_inputDevicesDisabled )
		return;

	if( m_isWaylandSession )
		disableInputDevicesWayland();
	else
		grabX11InputDevices();

	m_inputDevicesDisabled = true;
}



KeyboardShortcutTrapper* LinuxInputDeviceFunctions::createKeyboardShortcutTrapper( QObject* parent )
{
	return new LinuxKeyboardShortcutTrapper( parent );
}



void LinuxInputDeviceFunctions::grabX11InputDevices()
{
	if(m_x11GrabDisplay)
		return;

	auto* display = XOpenDisplay(nullptr);
	if(!display)
		return;

	m_x11GrabDisplay = display;

	int ndevices = 0;
	auto* devices = XIQueryDevice(display, XIAllDevices, &ndevices);

	for(int i = 0; i < ndevices; ++i)
	{
		const auto& dev = devices[i];

		// Grab only physical slave devices (skip master devices and XTEST virtual device)
		if(dev.use != XISlavePointer && dev.use != XISlaveKeyboard)
			continue;

		if(strstr(dev.name, "XTEST") != nullptr)
			continue;

		if(strstr(dev.name, "XTEST") != nullptr)
			continue;

		XIEventMask mask{};
		mask.deviceid = dev.deviceid;
		mask.mask_len = 0;
		mask.mask = nullptr;

		XIGrabDevice(display, dev.deviceid, DefaultRootWindow(display),
					  CurrentTime, None, GrabModeAsync, GrabModeAsync,
					  False, &mask);
	}

	XIFreeDeviceInfo(devices);
	XFlush(display);
}



void LinuxInputDeviceFunctions::ungrabX11InputDevices()
{
	if(m_x11GrabDisplay == nullptr)
		return;

	auto* display = static_cast<Display*>(m_x11GrabDisplay);

	int ndevices = 0;
	auto* devices = XIQueryDevice(display, XIAllDevices, &ndevices);

	for(int i = 0; i < ndevices; ++i)
	{
		const auto& dev = devices[i];

		if(dev.use != XISlavePointer && dev.use != XISlaveKeyboard)
			continue;

		if(strstr( dev.name, "XTEST" ) != nullptr)
			continue;

		XIUngrabDevice(display, dev.deviceid, CurrentTime);
	}

	XIFreeDeviceInfo(devices);

	XCloseDisplay(display);
	m_x11GrabDisplay = nullptr;
}



// ---------------------------------------------------------------------------
// Wayland input device blocking via privileged daemon (EVIOCGRAB)
// ---------------------------------------------------------------------------

void LinuxInputDeviceFunctions::disableInputDevicesWayland()
{
	if (m_inputBlockHelper)
		m_inputBlockHelper->block();
}



void LinuxInputDeviceFunctions::enableInputDevicesWayland()
{
	if (m_inputBlockHelper)
		m_inputBlockHelper->unblock();
}
