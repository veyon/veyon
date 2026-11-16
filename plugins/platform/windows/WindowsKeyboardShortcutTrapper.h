// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "KeyboardShortcutTrapper.h"

#include <QMutex>
#include <QTimer>

class WindowsKeyboardShortcutTrapper : public KeyboardShortcutTrapper
{
	Q_OBJECT
public:
	WindowsKeyboardShortcutTrapper(QObject* parent = nullptr);
	~WindowsKeyboardShortcutTrapper() override;

	void setEnabled(bool on) override;

private:
	void forwardTrappedShortcuts();

	static constexpr int PollTimerInterval = 10;

	static QMutex s_refCntMutex;
	static int s_refCnt;

	bool m_enabled{false};
	QTimer m_pollTimer;
};
