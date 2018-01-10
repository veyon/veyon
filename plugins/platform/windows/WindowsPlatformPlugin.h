/*
 * WindowsPlatformPlugin.h - declaration of WindowsPlatformPlugin class
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef WINDOWS_PLATFORM_PLUGIN_H
#define WINDOWS_PLATFORM_PLUGIN_H

#include "PluginInterface.h"
#include "PlatformPluginInterface.h"
#include "WindowsCoreFunctions.h"
#include "WindowsInputDeviceFunctions.h"
#include "WindowsNetworkFunctions.h"
#include "WindowsServiceFunctions.h"
#include "WindowsUserFunctions.h"

class WindowsPlatformPlugin : public QObject, PlatformPluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon.Veyon.Plugins.WindowsPlatform")
	Q_INTERFACES(PluginInterface PlatformPluginInterface)
public:
	WindowsPlatformPlugin( QObject* parent = nullptr );
	~WindowsPlatformPlugin() override;

	Plugin::Uid uid() const override
	{
		return "1baa01e0-02d6-4494-a766-788f5b225991";
	}

	QString version() const override
	{
		return QStringLiteral( "1.0" );
	}

	QString name() const override
	{
		return QStringLiteral( "WindowsPlatformPlugin" );
	}

	QString description() const override
	{
		return tr( "Plugin implementing abstract functions for the Windows platform" );
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
		return m_windowsCoreFunctions;
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

	PlatformUserFunctions& userFunctions() override
	{
		return m_windowsUserFunctions;
	}

private:
	WindowsCoreFunctions m_windowsCoreFunctions;
	WindowsInputDeviceFunctions m_windowsInputDeviceFunctions;
	WindowsNetworkFunctions m_windowsNetworkFunctions;
	WindowsServiceFunctions m_windowsServiceFunctions;
	WindowsUserFunctions m_windowsUserFunctions;

};

#endif // WINDOWS_PLATFORM_PLUGIN_H
