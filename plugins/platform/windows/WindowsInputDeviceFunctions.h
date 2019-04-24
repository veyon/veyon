/*
 * WindowsInputDeviceFunctions.h - declaration of WindowsInputDeviceFunctions class
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

#pragma once

#include <interception.h>

#include "PlatformInputDeviceFunctions.h"

// clazy:excludeall=copyable-polymorphic

class WindowsInputDeviceFunctions : public PlatformInputDeviceFunctions
{
public:
	WindowsInputDeviceFunctions();
	~WindowsInputDeviceFunctions();

	void enableInputDevices() override;
	void disableInputDevices() override;

	KeyboardShortcutTrapper* createKeyboardShortcutTrapper( QObject* parent ) override;

	static void checkInterceptionInstallation();

private:
	void enableInterception();
	void disableInterception();
	void initHIDServiceStatus();
	void stopHIDService();
	void restoreHIDService();

	static bool installInterception();
	static bool uninstallInterception();
	static int interceptionInstaller( const QString& argument );

	bool m_inputDevicesDisabled;
	InterceptionContext m_interceptionContext;
	QString m_hidServiceName;
	bool m_hidServiceStatusInitialized;
	bool m_hidServiceActivated;

};
