/*
 * LinuxInputDeviceFunctions.h - declaration of LinuxInputDeviceFunctions class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef LINUX_INPUT_DEVICE_FUNCTIONS_H
#define LINUX_INPUT_DEVICE_FUNCTIONS_H

#include "PlatformInputDeviceFunctions.h"

// clazy:excludeall=copyable-polymorphic

class LinuxInputDeviceFunctions : public PlatformInputDeviceFunctions
{
public:
	LinuxInputDeviceFunctions();
	~LinuxInputDeviceFunctions();

	void enableInputDevices() override;
	void disableInputDevices() override;

	KeyboardShortcutTrapper* createKeyboardShortcutTrapper( QObject* parent ) override;

	bool configureSoftwareSAS( bool enabled ) override;

private:
	void setEmptyKeyMapTable();
	void restoreKeyMapTable();

	bool m_inputDevicesDisabled;
	void* m_origKeyTable;
	int m_keyCodeMin;
	int m_keyCodeMax;
	int m_keyCodeCount;
	int m_keySymsPerKeyCode;

};

#endif // LINUX_INPUT_DEVICE_FUNCTIONS_H
