/*
 * BuiltinX11VncServer.h - declaration of BuiltinX11VncServer class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef BUILTIN_X11VNC_SERVER_H
#define BUILTIN_X11VNC_SERVER_H

#include "PluginInterface.h"
#include "VncServerPluginInterface.h"
#include "X11VncConfiguration.h"

class BuiltinX11VncServer : public QObject, VncServerPluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon.Veyon.Plugins.BuiltinX11VncServer")
	Q_INTERFACES(PluginInterface VncServerPluginInterface)
public:
	BuiltinX11VncServer();
	~BuiltinX11VncServer() override;

	Plugin::Uid uid() const override
	{
		return "39d7a07f-94db-4912-aa1a-c4df8aee3879";
	}

	QString version() const override
	{
		return QStringLiteral( "1.0" );
	}

	QString name() const override
	{
		return QStringLiteral( "BuiltinX11VncServer" );
	}

	QString description() const override
	{
		return tr( "Builtin VNC server (x11vnc)" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Doerffel" );
	}

	Plugin::Flags flags() const override
	{
		return Plugin::ProvidesDefaultImplementation;
	}

	QWidget* configurationWidget() override;

	void run( int serverPort, const QString& password ) override;

	QString configuredPassword() override
	{
		return QString();
	}

private:
	X11VncConfiguration m_configuration;

};

#endif // BUILTIN_X11VNC_SERVER_H
