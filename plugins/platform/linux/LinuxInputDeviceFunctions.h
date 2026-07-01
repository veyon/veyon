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
	void setEmptyKeyMapTable();
	void restoreKeyMapTable();

	void disableInputDevicesWayland();
	void enableInputDevicesWayland();

	bool m_inputDevicesDisabled{false};
	void* m_origKeyTable{nullptr};
	int m_keyCodeMin{0};
	int m_keyCodeMax{0};
	int m_keyCodeCount{0};
	int m_keySymsPerKeyCode{0};

	const bool m_isWaylandSession;
	InputBlockHelper* m_inputBlockHelper{nullptr};
};
