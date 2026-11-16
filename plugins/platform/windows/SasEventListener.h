// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

#include <QThread>

#include "WindowsSmartObjects.h"

class SasEventListener : public QThread
{
	Q_OBJECT
public:
	using SendSas = void(WINAPI*)(BOOL asUser);

	static constexpr DWORD WaitPeriod = 1000;

	SasEventListener();
	~SasEventListener() override;

	void stop();

	void run() override;

private:
	HMODULE m_sasLibrary;
	SendSas m_sendSas;
	SmartHandle m_sasEvent;
	SmartHandle m_stopEvent;
};
