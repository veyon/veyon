/*
 * OsXPlatformPlugin.h - declaration of OsXPlatformPlugin class
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

#include "PluginInterface.h"
#include "PlatformPluginInterface.h"
#include "OsXCoreFunctions.h"
#include "OsXFilesystemFunctions.h"
#include "OsXInputDeviceFunctions.h"
#include "OsXNetworkFunctions.h"
#include "OsXServiceFunctions.h"
#include "OsXSessionFunctions.h"
#include "OsXUserFunctions.h"

class OsXPlatformPlugin : public QObject, PlatformPluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.OsXPlatform")
	Q_INTERFACES(PluginInterface PlatformPluginInterface)
public:
	explicit OsXPlatformPlugin( QObject* parent = nullptr );
	~OsXPlatformPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("557107e3-22f6-4637-92d8-c3eb08e2ed5a") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 0, 2 );
	}

	QString name() const override
	{
		return QStringLiteral( "OsXPlatformPlugin" );
	}

	QString description() const override
	{
		return tr( "Plugin implementing abstract functions for the macOS platform" );
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
		return m_coreFunctions;
	}

	PlatformFilesystemFunctions& filesystemFunctions() override
	{
		return m_filesystemFunctions;
	}

	PlatformInputDeviceFunctions& inputDeviceFunctions() override
	{
		return m_inputDeviceFunctions;
	}

	PlatformNetworkFunctions& networkFunctions() override
	{
		return m_networkFunctions;
	}

	PlatformServiceFunctions& serviceFunctions() override
	{
		return m_serviceFunctions;
	}

	PlatformSessionFunctions& sessionFunctions() override
	{
		return m_sessionFunctions;
	}

	PlatformUserFunctions& userFunctions() override
	{
		return m_userFunctions;
	}

private:
	OsXCoreFunctions m_coreFunctions{};
	OsXFilesystemFunctions m_filesystemFunctions{};
	OsXInputDeviceFunctions m_inputDeviceFunctions{};
	OsXNetworkFunctions m_networkFunctions{};
	OsXServiceFunctions m_serviceFunctions{};
	OsXSessionFunctions m_sessionFunctions{};
	OsXUserFunctions m_userFunctions{};
};
