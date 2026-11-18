// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "QuickConfigurationPluginInterface.h"

#include "ConfigurationPagePluginInterface.h"
#include "PluginInterface.h"
#include "PlatformPluginInterface.h"
#include "WindowsCoreFunctions.h"
#include "WindowsFilesystemFunctions.h"
#include "WindowsInputDeviceFunctions.h"
#include "WindowsNetworkFunctions.h"
#include "WindowsServiceFunctions.h"
#include "WindowsSessionFunctions.h"
#include "WindowsUserFunctions.h"

class QWindow;

class WindowsPlatformPlugin : public QObject, PlatformPluginInterface, PluginInterface, ConfigurationPagePluginInterface,
		public QuickConfigurationPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.WindowsPlatform")
	Q_INTERFACES(QuickConfigurationPluginInterface PluginInterface PlatformPluginInterface ConfigurationPagePluginInterface)
public:
	QObject* createQuickConfiguration(Configuration::Object* draft, QObject* parent) override;

	WindowsPlatformPlugin( QObject* parent = nullptr );
	~WindowsPlatformPlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("1baa01e0-02d6-4494-a766-788f5b225991")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("WindowsPlatformPlugin");
	}

	QString description() const override
	{
		return tr("Plugin implementing abstract functions for the Windows platform");
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
		return m_windowsCoreFunctions;
	}

	PlatformFilesystemFunctions& filesystemFunctions() override
	{
		return m_windowsFilesystemFunctions;
	}

	PlatformInputDeviceFunctions& inputDeviceFunctions() override
	{
		return m_windowsInputDeviceFunctions;
	}

	PlatformNetworkFunctions& networkFunctions() override
	{
		return m_windowsNetworkFunctions;
	}

	PlatformServiceFunctions& serviceFunctions() override
	{
		return m_windowsServiceFunctions;
	}

	PlatformSessionFunctions& sessionFunctions() override
	{
		return m_windowsSessionFunctions;
	}

	PlatformUserFunctions& userFunctions() override
	{
		return m_windowsUserFunctions;
	}

	ConfigurationPage* createConfigurationPage() override;

private:
	WindowsCoreFunctions m_windowsCoreFunctions{};
	WindowsFilesystemFunctions m_windowsFilesystemFunctions{};
	WindowsInputDeviceFunctions m_windowsInputDeviceFunctions{};
	WindowsNetworkFunctions m_windowsNetworkFunctions{};
	WindowsServiceFunctions m_windowsServiceFunctions{};
	WindowsSessionFunctions m_windowsSessionFunctions{};
	WindowsUserFunctions m_windowsUserFunctions{};
	QWindow* m_dummyWindow{nullptr};
};
