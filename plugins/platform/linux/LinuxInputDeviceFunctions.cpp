// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "InputBlockHelper.h"
#include "LinuxInputDeviceFunctions.h"
#include "LinuxKeyboardShortcutTrapper.h"
#include "PlatformServiceFunctions.h"


LinuxInputDeviceFunctions::LinuxInputDeviceFunctions() :
	m_isWaylandSession(qEnvironmentVariableIsSet("WAYLAND_DISPLAY"))
{
	if (m_isWaylandSession)
	{
		m_inputBlockHelper = new InputBlockHelper;
	}
}

LinuxInputDeviceFunctions::~LinuxInputDeviceFunctions()
{
	delete m_inputBlockHelper;
}

#include <X11/XKBlib.h>


void LinuxInputDeviceFunctions::enableInputDevices()
{
	if (m_isWaylandSession)
	{
		// Always send unblock — the daemon tracks state, unblock is idempotent
		enableInputDevicesWayland();
		m_inputDevicesDisabled = false;
		return;
	}

	if (m_inputDevicesDisabled == false)
		return;

	restoreKeyMapTable();
	m_inputDevicesDisabled = false;
}



void LinuxInputDeviceFunctions::disableInputDevices()
{
	if (m_inputDevicesDisabled)
		return;

	if (m_isWaylandSession)
		disableInputDevicesWayland();
	else
		setEmptyKeyMapTable();

	m_inputDevicesDisabled = true;
}



KeyboardShortcutTrapper* LinuxInputDeviceFunctions::createKeyboardShortcutTrapper(QObject* parent)
{
	return new LinuxKeyboardShortcutTrapper(parent);
}



void LinuxInputDeviceFunctions::setEmptyKeyMapTable()
{
	if (m_origKeyTable)
		XFree(m_origKeyTable);

	auto display = XOpenDisplay(nullptr);
	if (display == nullptr)
	{
		vCritical() << "cannot open X display";
		return;
	}

	XDisplayKeycodes(display, &m_keyCodeMin, &m_keyCodeMax);
	m_keyCodeCount = m_keyCodeMax - m_keyCodeMin + 1;

	m_origKeyTable = XGetKeyboardMapping(display, ::KeyCode(m_keyCodeMin), m_keyCodeCount, &m_keySymsPerKeyCode);

	auto newKeyTable = XGetKeyboardMapping(display, ::KeyCode(m_keyCodeMin), m_keyCodeCount, &m_keySymsPerKeyCode);

	for (int i = 0; i < m_keyCodeCount * m_keySymsPerKeyCode; i++)
		newKeyTable[i] = 0;

	XChangeKeyboardMapping(display, m_keyCodeMin, m_keySymsPerKeyCode, newKeyTable, m_keyCodeCount);
	XFlush(display);
	XFree(newKeyTable);
	XCloseDisplay(display);
}



void LinuxInputDeviceFunctions::restoreKeyMapTable()
{
	Display* display = XOpenDisplay(nullptr);

	XChangeKeyboardMapping(display, m_keyCodeMin, m_keySymsPerKeyCode,
						   static_cast<::KeySym*>(m_origKeyTable), m_keyCodeCount);

	XFlush(display);
	XCloseDisplay(display);

	XFree(m_origKeyTable);
	m_origKeyTable = nullptr;
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
