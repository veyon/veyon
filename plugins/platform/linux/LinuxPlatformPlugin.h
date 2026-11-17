// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ConfigurationPagePluginInterface.h"
#include "LinuxCoreFunctions.h"
#include "LinuxFilesystemFunctions.h"
#include "LinuxInputDeviceFunctions.h"
#include "LinuxNetworkFunctions.h"
#include "LinuxSecretFunctions.h"
#include "LinuxServiceFunctions.h"
#include "LinuxSessionFunctions.h"
#include "LinuxUserFunctions.h"
#include "PlatformPluginInterface.h"
#include "PluginInterface.h"

class LinuxPlatformPlugin : public QObject, PlatformPluginInterface, PluginInterface, ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.LinuxPlatform")
	Q_INTERFACES(PluginInterface PlatformPluginInterface ConfigurationPagePluginInterface)
public:
	explicit LinuxPlatformPlugin(QObject* parent = nullptr);
	~LinuxPlatformPlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("63928a8a-4c51-4bfd-888e-9e13c6f3907a")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("LinuxPlatformPlugin");
	}

	QString description() const override
	{
		return tr("Plugin implementing abstract functions for the Linux platform");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	Plugin::Flags flags() const override
	{
		return Plugin::ProvidesDefaultImplementation;
	}

	PlatformCoreFunctions& coreFunctions() override
	{
		return m_linuxCoreFunctions;
	}

	PlatformFilesystemFunctions& filesystemFunctions() override
	{
		return m_linuxFilesystemFunctions;
	}

	PlatformInputDeviceFunctions& inputDeviceFunctions() override
	{
		return m_linuxInputDeviceFunctions;
	}

	PlatformNetworkFunctions& networkFunctions() override
	{
		return m_linuxNetworkFunctions;
	}

	PlatformSecretFunctions& secretFunctions() override
	{
		return m_linuxSecretFunctions;
	}

	PlatformServiceFunctions& serviceFunctions() override
	{
		return m_linuxServiceFunctions;
	}

	PlatformSessionFunctions& sessionFunctions() override
	{
		return m_linuxSessionFunctions;
	}

	PlatformUserFunctions& userFunctions() override
	{
		return m_linuxUserFunctions;
	}

	ConfigurationPage* createConfigurationPage() override;

	static void abort(int signal);
	static QString formattedBacktraceString();

private:
	LinuxCoreFunctions m_linuxCoreFunctions{};
	LinuxFilesystemFunctions m_linuxFilesystemFunctions{};
	LinuxInputDeviceFunctions m_linuxInputDeviceFunctions{};
	LinuxNetworkFunctions m_linuxNetworkFunctions{};
	LinuxSecretFunctions m_linuxSecretFunctions{};
	LinuxServiceFunctions m_linuxServiceFunctions{};
	LinuxSessionFunctions m_linuxSessionFunctions{};
	LinuxUserFunctions m_linuxUserFunctions{};
};
