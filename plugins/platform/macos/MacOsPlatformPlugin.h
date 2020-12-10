/*
 * MacOsPlatformPlugin.h - declaration of MacOsPlatformPlugin class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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
#include "MacOsCoreFunctions.h"
#include "MacOsFilesystemFunctions.h"
#include "MacOsInputDeviceFunctions.h"
#include "MacOsNetworkFunctions.h"
#include "MacOsServiceFunctions.h"
#include "MacOsSessionFunctions.h"
#include "MacOsUserFunctions.h"

class MacOsPlatformPlugin : public QObject, PlatformPluginInterface, PluginInterface, ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.MacOsPlatform")
	Q_INTERFACES(PluginInterface PlatformPluginInterface ConfigurationPagePluginInterface)
public:
	explicit MacOsPlatformPlugin( QObject* parent = nullptr );
	~MacOsPlatformPlugin() override;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("E0BAC73E-55E2-4F89-98CF-221E2DD376D7");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "MacOsPlatformPlugin" );
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
		return QStringLiteral( "Pascâl Hartmann" );
	}

	Plugin::Flags flags() const override
	{
		return Plugin::ProvidesDefaultImplementation;
	}

	PlatformCoreFunctions& coreFunctions() override
	{
		return m_macOsCoreFunctions;
	}

	PlatformFilesystemFunctions& filesystemFunctions() override
	{
		return m_macOsFilesystemFunctions;
	}

	PlatformInputDeviceFunctions& inputDeviceFunctions() override
	{
		return m_macOsInputDeviceFunctions;
	}

	PlatformNetworkFunctions& networkFunctions() override
	{
		return m_macOsNetworkFunctions;
	}

	PlatformServiceFunctions& serviceFunctions() override
	{
		return m_macOsServiceFunctions;
	}

	PlatformSessionFunctions& sessionFunctions() override
	{
		return m_macOsSessionFunctions;
	}

	PlatformUserFunctions& userFunctions() override
	{
		return m_macOsUserFunctions;
	}

	ConfigurationPage* createConfigurationPage() override;

private:
	MacOsCoreFunctions m_macOsCoreFunctions{};
	MacOsFilesystemFunctions m_macOsFilesystemFunctions{};
	MacOsInputDeviceFunctions m_macOsInputDeviceFunctions{};
	MacOsNetworkFunctions m_macOsNetworkFunctions{};
	MacOsServiceFunctions m_macOsServiceFunctions{};
	MacOsSessionFunctions m_macOsSessionFunctions{};
	MacOsUserFunctions m_macOsUserFunctions{};

};
