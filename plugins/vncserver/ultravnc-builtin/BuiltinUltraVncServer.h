/*
 * BuiltinUltraVncServer.h - declaration of BuiltinUltraVncServer class
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include "UltraVncConfiguration.h"
#include "VncServerPluginInterface.h"

class LogoffEventFilter;

class BuiltinUltraVncServer : public QObject, VncServerPluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.BuiltinUltraVncServer")
	Q_INTERFACES(PluginInterface VncServerPluginInterface)
public:
	BuiltinUltraVncServer();
	~BuiltinUltraVncServer() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("39d7a07f-94db-4912-aa1a-c4df8aee3879") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "BuiltinUltraVncServer" );
	}

	QString description() const override
	{
		return tr( "Builtin VNC server (UltraVNC)" );
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

	QStringList supportedSessionTypes() const override
	{
		return { QStringLiteral("console"), QStringLiteral("rdp") };
	}

	QWidget* configurationWidget() override;

	void prepareServer() override;

	bool runServer( int serverPort, const Password& password ) override;

	int configuredServerPort() override
	{
		return -1;
	}

	Password configuredPassword() override
	{
		return {};
	}

	const UltraVncConfiguration& configuration() const
	{
		return m_configuration;
	}

	int serverPort() const
	{
		return m_serverPort;
	}

	const Password& password() const
	{
		return m_password;
	}

private:
	UltraVncConfiguration m_configuration;

	static constexpr auto DefaultServerPort = 5900;

	int m_serverPort;
	Password m_password;

	LogoffEventFilter* m_logoffEventFilter;

};
