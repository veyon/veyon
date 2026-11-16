// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

#include <QAbstractNativeEventFilter>

// event filter which makes ICA recognize logoff events etc.
class LogoffEventFilter : public QAbstractNativeEventFilter
{
public:
	LogoffEventFilter();

	bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

private:
	HANDLE m_shutdownEventHandle;
};
