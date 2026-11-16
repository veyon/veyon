// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

class VEYON_CORE_EXPORT KeyboardShortcutTrapper : public QObject
{
	Q_OBJECT
public:
	enum Shortcut {
		NoShortcut,
		AltTab,
		AltEsc,
		AltSpace,
		AltF4,
		CtrlEsc,
		SuperKeyDown,
		SuperKeyUp
	};
	Q_ENUM(Shortcut)

	explicit KeyboardShortcutTrapper(QObject* parent = nullptr) :
		QObject(parent)
	{
	}

	virtual void setEnabled(bool on) = 0;

Q_SIGNALS:
	void shortcutTrapped(KeyboardShortcutTrapper::Shortcut);
};
