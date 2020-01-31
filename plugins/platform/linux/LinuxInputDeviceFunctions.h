/*
 * LinuxInputDeviceFunctions.h - declaration of LinuxInputDeviceFunctions class
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "PlatformInputDeviceFunctions.h"

// clazy:excludeall=copyable-polymorphic

class LinuxInputDeviceFunctions : public PlatformInputDeviceFunctions
{
public:
	LinuxInputDeviceFunctions() = default;
	virtual ~LinuxInputDeviceFunctions() = default;

	void enableInputDevices() override;
	void disableInputDevices() override;

	KeyboardShortcutTrapper* createKeyboardShortcutTrapper( QObject* parent ) override;

private:
	void setEmptyKeyMapTable();
	void restoreKeyMapTable();

	bool m_inputDevicesDisabled{false};
	void* m_origKeyTable{nullptr};
	int m_keyCodeMin{0};
	int m_keyCodeMax{0};
	int m_keyCodeCount{0};
	int m_keySymsPerKeyCode{0};

};
