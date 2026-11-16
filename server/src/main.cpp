// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QGuiApplication>

#include "ComputerControlServer.h"


int main(int argc, char** argv)
{
	VeyonCore::setupApplicationParameters();

	QGuiApplication app(argc, argv);
	QGuiApplication::setDesktopFileName(QStringLiteral("io.veyon.veyon-server"));

	VeyonCore core(&app, VeyonCore::Component::Server, QStringLiteral("Server"));

	ComputerControlServer server(&core);
	if (server.start() == false)
	{
		vCritical() << "Failed to start server";
		return -1;
	}

	return core.exec();
}
