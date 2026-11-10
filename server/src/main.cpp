/*
 * main.cpp - main file for Veyon Server
 *
 * Copyright (c) 2006-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QGuiApplication>
#include <QFile>

#include <cstring>

#include "rfb/rfbproto.h"

#include "ComputerControlServer.h"
#include "PluginManager.h"
#include "VncServerPluginInterface.h"

namespace
{
int runServer(QCoreApplication& app, bool greeter)
{
	VeyonCore core(&app, VeyonCore::Component::Server,
				   greeter ? QStringLiteral("GreeterServer") : QStringLiteral("Server"));
	ComputerControlServer server(&core, greeter);
	if (!server.start())
	{
		vCritical() << "Failed to start server";
		return -1;
	}
	return core.exec();
}

#ifdef Q_OS_LINUX
int runGreeterBackend(QCoreApplication& app)
{
	const auto arguments = app.arguments();
	bool portOk = false;
	const auto port = arguments.value(3).toUShort(&portOk);
	const auto backend = arguments.value(2);
	if (arguments.size() != 4 || !portOk || port == 0 ||
		(backend != QLatin1String("x11") && backend != QLatin1String("headless")))
	{
		return 1;
	}
	QFile input;
	if (!input.open(stdin, QIODevice::ReadOnly))
	{
		return 1;
	}
	const auto password = input.read(MAXPWLEN);
	if (password.size() != MAXPWLEN)
	{
		return 1;
	}
	VeyonCore core(&app, VeyonCore::Component::Server, QStringLiteral("GreeterCapture"));
	// This process only serves internal RFB on loopback. It never creates a
	// ComputerControlServer or a second feature-worker listener.
	const auto uid = backend == QLatin1String("x11") ?
		QUuid{QStringLiteral("39d7a07f-94db-4912-aa1a-c4df8aee3879")} :
		QUuid{QStringLiteral("f626f759-7691-45c0-bd4a-37171d98d219")};
	for (auto object : std::as_const(VeyonCore::pluginManager().pluginObjects()))
	{
		auto plugin = qobject_cast<PluginInterface*>(object);
		auto vnc = qobject_cast<VncServerPluginInterface*>(object);
		if (plugin && vnc && plugin->uid() == uid)
		{
			vInfo() << "Starting isolated greeter capture backend" << backend;
			vnc->prepareServer();
			return vnc->runServer(port, CryptoCore::SecureArray(password)) ? 0 : 1;
		}
	}
	vCritical() << "Required greeter backend plugin is not installed:" << backend;
	return 1;
}
#endif
}


int main( int argc, char **argv )
{
	VeyonCore::setupApplicationParameters();

#ifdef Q_OS_LINUX
	if (argc > 1 && (std::strcmp(argv[1], "--greeter") == 0 || std::strcmp(argv[1], "--greeter-vnc") == 0))
	{
		// Keep the authenticated control endpoint independent of a working Qt
		// display connection. GUI feature workers still inherit the real display.
		const bool capture = std::strcmp(argv[1], "--greeter-vnc") == 0;
		QCoreApplication app(argc, argv);
		if (capture)
		{
			return runGreeterBackend(app);
		}
		return app.arguments().size() == 2 ? runServer(app, true) : 1;
	}
#endif

	QGuiApplication app( argc, argv );
	QGuiApplication::setDesktopFileName(QStringLiteral("io.veyon.veyon-server"));

	return runServer(app, false);
}
