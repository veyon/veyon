// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "KeyboardShortcutTrapper.h"

class LinuxKeyboardShortcutTrapper : public KeyboardShortcutTrapper
{
	Q_OBJECT
public:
	explicit LinuxKeyboardShortcutTrapper(QObject* parent = nullptr) :
		KeyboardShortcutTrapper(parent)
	{
	}

	~LinuxKeyboardShortcutTrapper() override = default;

	void setEnabled(bool on) override
	{
		Q_UNUSED(on);
	}
};
