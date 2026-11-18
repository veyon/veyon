// SPDX-FileCopyrightText: 2004-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QWindow>


#include "VeyonMaster.h"
#include "MainWindow.h"


int main(int argc, char** argv)
{
	VeyonCore::setupApplicationParameters();

	QApplication app( argc, argv );

	VeyonCore core(&app, VeyonCore::Component::Master, QStringLiteral("Master"));


	if (MainWindow::initAuthentication() == false ||
		MainWindow::initAccessControl() == false)
	{
		return -1;
	}

	VeyonMaster masterCore(&core);

	if (!masterCore.mainQmlWindow()) return -1;
	masterCore.mainQmlWindow()->show();

	return core.exec();
}
