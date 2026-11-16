// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PlatformServiceFunctions.h"

// clazy:excludeall=copyable-polymorphic

class LinuxServiceFunctions : public PlatformServiceFunctions
{
public:
	QString veyonServiceName() const override;

	bool isRegistered(const QString& name) override;
	bool isRunning(const QString& name) override;
	bool start(const QString& name) override;
	bool stop(const QString& name) override;
	bool install(const QString& name, const QString& serviceFilePath,
				 StartMode startMode, const QString& displayName) override;
	bool uninstall(const QString& name) override;
	bool setStartMode(const QString& name, StartMode startMode) override;
	bool runAsService(const QString& name, const ServiceEntryPoint& serviceEntryPoint) override;
	void manageServerInstances() override;
};
