/*
 * LinuxPlatformPlugin.h - declaration of LinuxPlatformPlugin class
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include "ConfigurationPagePluginInterface.h"
#include "PluginInterface.h"
#include "PlatformPluginInterface.h"
#include "LinuxCoreFunctions.h"
#include "LinuxFilesystemFunctions.h"
#include "LinuxInputDeviceFunctions.h"
#include "LinuxNetworkFunctions.h"
#include "LinuxServiceFunctions.h"
#include "LinuxSessionFunctions.h"
#include "LinuxUserFunctions.h"

class LinuxPlatformPlugin : public QObject, PlatformPluginInterface, PluginInterface, ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.LinuxPlatform")
	Q_INTERFACES(PluginInterface PlatformPluginInterface ConfigurationPagePluginInterface)
public:
	explicit LinuxPlatformPlugin( QObject* parent = nullptr );
	~LinuxPlatformPlugin() override;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("63928a8a-4c51-4bfd-888e-9e13c6f3907a");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "LinuxPlatformPlugin" );
	}

	QString description() const override
	{
		return tr( "Plugin implementing abstract functions for the Linux platform" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Junghans" );
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

private:
	LinuxCoreFunctions m_linuxCoreFunctions{};
	LinuxFilesystemFunctions m_linuxFilesystemFunctions{};
	LinuxInputDeviceFunctions m_linuxInputDeviceFunctions{};
	LinuxNetworkFunctions m_linuxNetworkFunctions{};
	LinuxServiceFunctions m_linuxServiceFunctions{};
	LinuxSessionFunctions m_linuxSessionFunctions{};
	LinuxUserFunctions m_linuxUserFunctions{};

};
