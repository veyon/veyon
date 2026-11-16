// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PlatformPluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class KeyboardShortcutTrapper;

class PlatformInputDeviceFunctions
{
public:
	virtual void enableInputDevices() = 0;
	virtual void disableInputDevices() = 0;

	virtual KeyboardShortcutTrapper* createKeyboardShortcutTrapper(QObject* parent) = 0;
};
