// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PlatformPluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class PlatformServiceFunctions
{
public:
	using ServiceEntryPoint = std::function<void(void)>;

	enum class StartMode {
		Disabled,
		Manual,
		Auto
	};

	virtual ~PlatformServiceFunctions() = default;

	virtual QString veyonServiceName() const = 0;

	virtual bool isRegistered(const QString& name) = 0;
	virtual bool isRunning(const QString& name) = 0;
	virtual bool start(const QString& name) = 0;
	virtual bool stop(const QString& name) = 0;
	virtual bool install(const QString& name, const QString& filePath,
						 StartMode startMode, const QString& displayName) = 0;
	virtual bool uninstall(const QString& name) = 0;
	virtual bool setStartMode(const QString& name, StartMode startMode) = 0;
	virtual bool runAsService(const QString& name, const ServiceEntryPoint& serviceEntryPoint) = 0;
	virtual void manageServerInstances() = 0;
};
