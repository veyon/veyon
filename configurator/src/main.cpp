// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ConfiguratorController.h"
#include "Logger.h"
#include "PlatformCoreFunctions.h"
#include "QuickUi.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"

int main(int argc, char** argv)
{
	VeyonCore::setupApplicationParameters();
	QApplication app(argc, argv);
	VeyonCore core(&app, VeyonCore::Component::Configurator, QStringLiteral("Configurator"));
	if (qEnvironmentVariableIntValue("VEYON_CONFIGURATOR_NO_ELEVATION") == 0 &&
		!VeyonCore::platform().coreFunctions().isRunningAsAdmin() && app.arguments().size() <= 1)
	{
		if (VeyonCore::platform().coreFunctions().runProgramAsAdmin(QCoreApplication::applicationFilePath(), {QStringLiteral("-elevated")})) return 0;
		QuickUi::showError(QObject::tr("Insufficient privileges"), QObject::tr("Could not start with administrative privileges. Please make sure a sudo-like program is installed for your desktop environment. The program will run with normal user privileges."));
	}
	if (!VeyonConfiguration().isStoreWritable() && VeyonCore::config().logLevel() != Logger::LogLevel::Debug)
	{
		QuickUi::showError(QObject::tr("Configuration not writable"), QObject::tr("The local configuration backend is not writable. Please run Veyon Configurator with higher privileges."));
		return -1;
	}
	ConfiguratorController controller;
	auto engine = QuickUi::createEngine(&app);
	engine->rootContext()->setContextProperty(QStringLiteral("configurator"), &controller);
	engine->load(QUrl(QStringLiteral("qrc:/configurator/Configurator.qml")));
	if (engine->rootObjects().isEmpty()) return -1;
	return core.exec();
}
