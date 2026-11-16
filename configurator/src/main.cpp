// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QMessageBox>

#include "Logger.h"
#include "MainWindow.h"
#include "PlatformCoreFunctions.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"



int main(int argc, char** argv)
{
	VeyonCore::setupApplicationParameters();

	QApplication app(argc, argv);

	VeyonCore core(&app, VeyonCore::Component::Configurator, QStringLiteral("Configurator"));

	// make sure to run as admin
	if (qEnvironmentVariableIntValue("VEYON_CONFIGURATOR_NO_ELEVATION") == 0 &&
		VeyonCore::platform().coreFunctions().isRunningAsAdmin() == false &&
		app.arguments().size() <= 1)
	{
		if (VeyonCore::platform().coreFunctions().runProgramAsAdmin(QCoreApplication::applicationFilePath(), {QStringLiteral("-elevated")}))
		{
			return 0;
		}

		QMessageBox::warning(nullptr, MainWindow::tr("Insufficient privileges"),
							 MainWindow::tr("Could not start with administrative privileges. "
											"Please make sure a sudo-like program is installed "
											"for your desktop environment! The program will "
											"be run with normal user privileges."));
	}

	if (VeyonConfiguration().isStoreWritable() == false &&
		VeyonCore::config().logLevel() != Logger::LogLevel::Debug)
	{
		QMessageBox::critical(nullptr,
							  MainWindow::tr("Configuration not writable"),
							  MainWindow::tr("The local configuration backend reported that the "
											 "configuration is not writable! Please run Veyon "
											 "Configurator with higher privileges."));
		return -1;
	}

	// now create the main window
	auto mainWindow = new MainWindow;
	mainWindow->show();

	return core.exec();
}
