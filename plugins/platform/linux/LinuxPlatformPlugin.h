/*
 * LinuxPlatformPlugin.h - declaration of LinuxPlatformPlugin class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef LINUX_PLATFORM_PLUGIN_H
#define LINUX_PLATFORM_PLUGIN_H

#include "PluginInterface.h"
#include "PlatformPluginInterface.h"
#include "LinuxCoreFunctions.h"
#include "LinuxNetworkFunctions.h"
#include "LinuxServiceFunctions.h"
#include "LinuxUserFunctions.h"

class LinuxPlatformPlugin : public QObject, PlatformPluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon.Veyon.Plugins.LinuxPlatform")
	Q_INTERFACES(PluginInterface PlatformPluginInterface)
public:
	LinuxPlatformPlugin( QObject* parent = nullptr );
	~LinuxPlatformPlugin() override;

	Plugin::Uid uid() const override
	{
		return "63928a8a-4c51-4bfd-888e-9e13c6f3907a";
	}

	QString version() const override
	{
		return QStringLiteral( "1.0" );
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

	PlatformNetworkFunctions& networkFunctions() override
	{
		return m_linuxNetworkFunctions;
	}

	PlatformServiceFunctions& serviceFunctions() override
	{
		return m_linuxServiceFunctions;
	}

	PlatformUserFunctions& userFunctions() override
	{
		return m_linuxUserFunctions;
	}

private:
	LinuxCoreFunctions m_linuxCoreFunctions;
	LinuxNetworkFunctions m_linuxNetworkFunctions;
	LinuxServiceFunctions m_linuxServiceFunctions;
	LinuxUserFunctions m_linuxUserFunctions;

};

#endif // LINUX_PLATFORM_PLUGIN_H
