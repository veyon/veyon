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

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QTimer>

#include "ComputerControlServer.h"

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
#include <dispatch/dispatch.h>
#endif


int main( int argc, char **argv )
{
	VeyonCore::setupApplicationParameters();

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
	const QFileInfo execInfo(QString::fromLocal8Bit(argv[0]));
	const QString modulesBase = QDir(execInfo.absoluteDir()).absoluteFilePath(QStringLiteral("../Frameworks/openssl/ossl-modules"));
	const QString modulesCanonical = QDir(modulesBase).canonicalPath();
	if( modulesCanonical.isEmpty() == false )
	{
		qputenv("OPENSSL_MODULES", modulesCanonical.toUtf8());
	}
#endif

	QGuiApplication app( argc, argv );
	QGuiApplication::setDesktopFileName(QStringLiteral("io.veyon.veyon-server"));

	VeyonCore core( &app, VeyonCore::Component::Server, QStringLiteral("Server") );

	ComputerControlServer server( &core );
	if( server.start() == false )
	{
		vCritical() << "Failed to start server";
		return -1;
	}

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
	// On macOS, ScreenCaptureKit needs the main dispatch queue to be processed regularly
	// Qt's event loop doesn't always process it frequently enough, so we force it with a timer
	// We use dispatch_async instead of dispatch_sync to avoid potential deadlocks
	QTimer dispatchPumpTimer;
	QObject::connect(&dispatchPumpTimer, &QTimer::timeout, []() {
		static int counter = 0;
		counter++;
		// Ping the main dispatch queue to force processing
		dispatch_async(dispatch_get_main_queue(), ^{
			// Empty block - forces queue to wake up and process pending events
		});
		// Log every 100 iterations (every ~1 second at 10ms intervals)
		if (counter % 100 == 0) {
			fprintf(stderr, "[PUMP] Main queue pump active (iteration %d)\n", counter);
			fflush(stderr);
		}
	});
	dispatchPumpTimer.start(10); // Process every 10ms
	fprintf(stderr, "[PUMP] macOS: Started main dispatch queue pump timer\n");
	fflush(stderr);
#endif

	return core.exec();
}
