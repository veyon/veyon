// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <guiddef.h>

// clazy:excludeall=copyable-polymorphic

class WindowsDeviceFunctions
{
public:
	enum class State {
		Disabled,
		Enabled
	};

	struct Device
	{
		QString instanceId;
		QString name;
	};

	using DeviceList = QList<Device>;

	static bool setDeviceState(const Device& device, State state);
	static bool setDevicesState(const DeviceList& devices, State state);

	static DeviceList findDevicesByClass(const GUID& classGuid);

	static DeviceList findKeyboardDevices();
	static DeviceList findMouseDevices();
	static DeviceList findTouchDevices();
};
