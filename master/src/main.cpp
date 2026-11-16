// SPDX-FileCopyrightText: 2004-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QSplashScreen>

#include "DocumentationFigureCreator.h"
#include "MainWindow.h"
#include "VeyonMaster.h"


int main(int argc, char** argv)
{
	VeyonCore::setupApplicationParameters();

	QApplication app(argc, argv);
	app.connect(&app, &QApplication::lastWindowClosed, &QApplication::quit);

	VeyonCore core(&app, VeyonCore::Component::Master, QStringLiteral("Master"));

#ifdef VEYON_DEBUG
	if (qEnvironmentVariableIsSet("VEYON_MASTER_CREATE_DOC_FIGURES"))
	{
		DocumentationFigureCreator().run();
		return 0;
	}
#endif

	QSplashScreen splashScreen(QPixmap(QStringLiteral(":/master/splash.png")));
	splashScreen.show();

	if (MainWindow::initAuthentication() == false ||
		MainWindow::initAccessControl() == false)
	{
		return -1;
	}

	VeyonMaster masterCore(&core);

	// hide splash-screen as soon as main-window is shown
	splashScreen.finish(masterCore.mainWindow());

	masterCore.mainWindow()->show();

	return core.exec();
}
