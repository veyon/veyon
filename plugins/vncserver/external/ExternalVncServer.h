/*
 * ExternalVncServer.h - declaration of ExternalVncServer class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef EXTERNAL_VNC_SERVER_H
#define EXTERNAL_VNC_SERVER_H

#include "PluginInterface.h"
#include "VncServerPluginInterface.h"
#include "ExternalVncServerConfiguration.h"

class ExternalVncServer : public QObject, VncServerPluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.ExternalVncServer")
	Q_INTERFACES(PluginInterface VncServerPluginInterface)
public:
	ExternalVncServer( QObject* parent = nullptr );
	~ExternalVncServer() override;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("67dfc1c1-8f37-4539-a298-16e74e34fd8b");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "ExternalVncServer" );
	}

	QString description() const override
	{
		return tr( "External VNC server" );
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
		return Plugin::NoFlags;
	}

	void upgrade( const QVersionNumber& oldVersion ) override;

	QWidget* configurationWidget() override;

	void prepareServer() override;

	void runServer( int serverPort, const QString& password ) override;

	int configuredServerPort() override;

	QString configuredPassword() override;

private:
	enum {
		MaximumPlaintextPasswordLength = 64
	};

	ExternalVncServerConfiguration m_configuration;

};

#endif // EXTERNAL_VNC_SERVER_H
