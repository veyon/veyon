// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

#include "VeyonCore.h"

class WindowsServiceControl : public QObject
{
	Q_OBJECT
public:
	WindowsServiceControl(const QString& name);
	~WindowsServiceControl() override;

	bool isRegistered();
	bool isRunning();
	bool start();
	bool stop();
	bool install(const QString& filePath, const QString& displayName);
	bool uninstall();
	bool setStartType(int startType);

private:
	bool checkService() const;

	static constexpr auto ServiceActionDelay = 10000;
	static constexpr auto ServiceWaitSleepInterval = 1000;

	const QString m_name;
	SC_HANDLE m_serviceManager;
	SC_HANDLE m_serviceHandle;
};
