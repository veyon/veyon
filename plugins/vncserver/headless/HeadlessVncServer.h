/*
 * HeadlessVncServer.h - declaration of HeadlessVncServer class
 *
 * Copyright (c) 2020-2021 Tobias Junghans <tobydox@veyon.io>
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
#include "VncServerPluginInterface.h"
#include "HeadlessVncConfiguration.h"

struct HeadlessVncScreen;

class HeadlessVncServer : public QObject, VncServerPluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.HeadlessVncServer")
	Q_INTERFACES(PluginInterface VncServerPluginInterface)
public:
	explicit HeadlessVncServer( QObject* parent = nullptr );

	Plugin::Uid uid() const override
	{
		return QStringLiteral("f626f759-7691-45c0-bd4a-37171d98d219");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral( "HeadlessVncServer" );
	}

	QString description() const override
	{
		return tr( "Headless VNC server" );
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

	virtual QStringList supportedSessionTypes() const override
	{
		return {};
	}

	QWidget* configurationWidget() override
	{
		return nullptr;
	}

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

private:
	static constexpr auto DefaultFramebufferWidth = 640;
	static constexpr auto DefaultFramebufferHeight = 480;
	static constexpr auto DefaultSleepTime = 25;

	bool initScreen( HeadlessVncScreen* screen );
	bool initVncServer( int serverPort, const VncServerPluginInterface::Password& password,
						HeadlessVncScreen* screen );

	bool handleScreenChanges( HeadlessVncScreen* screen );

	HeadlessVncConfiguration m_configuration;

};
