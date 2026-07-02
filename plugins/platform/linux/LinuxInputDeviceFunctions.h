/*
 * LinuxInputDeviceFunctions.h - declaration of LinuxInputDeviceFunctions class
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 */

#pragma once

#include "PlatformInputDeviceFunctions.h"

class InputBlockHelper;

class LinuxInputDeviceFunctions : public PlatformInputDeviceFunctions
{
public:
	LinuxInputDeviceFunctions();
	~LinuxInputDeviceFunctions();

	void enableInputDevices() override;
	void disableInputDevices() override;

	KeyboardShortcutTrapper* createKeyboardShortcutTrapper( QObject* parent ) override;

private:
	void grabX11InputDevices();
	void ungrabX11InputDevices();

	void disableInputDevicesWayland();
	void enableInputDevicesWayland();

	bool m_inputDevicesDisabled{false};

	const bool m_isWaylandSession;
	InputBlockHelper* m_inputBlockHelper{nullptr};

	void* m_x11GrabDisplay{nullptr};
};
