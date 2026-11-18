// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>

#include "PlatformServiceFunctions.h"
#include "ServiceControl.h"
#include "QuickUi.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QGuiApplication>
#include <memory>
#include <QWindow>


ServiceControl::ServiceControl(const QString& name,
							   const QString& filePath,
							   const QString& displayName,
							   QWidget* parent) :
	QObject(parent),
	m_name(name),
	m_filePath(filePath),
	m_displayName(displayName),
	m_parent(parent)
{
}



bool ServiceControl::isServiceRegistered()
{
	return VeyonCore::platform().serviceFunctions().isRegistered(m_name);
}



bool ServiceControl::isServiceRunning()
{
	return VeyonCore::platform().serviceFunctions().isRunning(m_name);
}



void ServiceControl::startService()
{
	serviceControl(tr("Starting %1").arg(m_displayName),
				   QtConcurrent::run([this]() { VeyonCore::platform().serviceFunctions().start(m_name); }));
}



void ServiceControl::stopService()
{
	serviceControl(tr("Stopping %1").arg(m_displayName),
				   QtConcurrent::run([this]() { VeyonCore::platform().serviceFunctions().stop(m_name); }));
}



void ServiceControl::restartService()
{
	serviceControl(tr("Restarting %1").arg(m_displayName),
				   QtConcurrent::run([this]() {
		VeyonCore::platform().serviceFunctions().stop(m_name);
		VeyonCore::platform().serviceFunctions().start(m_name);
	}));
}



void ServiceControl::registerService()
{
	serviceControl(tr("Registering %1").arg(m_displayName),
				   QtConcurrent::run([this]() { VeyonCore::platform().serviceFunctions().install(m_name,
																								 m_filePath,
																								 PlatformServiceFunctions::StartMode::Auto,
																								 m_displayName); }));
}



void ServiceControl::unregisterService()
{
	serviceControl(tr("Unregistering %1").arg(m_displayName),
				   QtConcurrent::run([this]() { VeyonCore::platform().serviceFunctions().uninstall(m_name); }));
}



void ServiceControl::serviceControl(const QString& title, const Operation& operation)
{
	if (m_parent || (qobject_cast<QGuiApplication*>(QCoreApplication::instance()) && QGuiApplication::focusWindow()))
	{
		graphicalFeedback(title, operation);
	}
	else
	{
		textFeedback(title, operation);
	}
}



void ServiceControl::graphicalFeedback(const QString& title, const Operation& operation)
{
	const auto parentWindow = QGuiApplication::focusWindow();
	std::unique_ptr<QQmlApplicationEngine> engine(QuickUi::createEngine());
	auto request = new QuickDialogRequest(QStringLiteral("busy"),
		{{QStringLiteral("title"), tr("Service control")}, {QStringLiteral("text"), title}}, engine.get());
	engine->rootContext()->setContextProperty(QStringLiteral("dialogRequest"), request);
	engine->load(QUrl(QStringLiteral("qrc:/quick/GenericDialog.qml")));
	if (!engine->rootObjects().isEmpty())
		if (auto window = qobject_cast<QWindow*>(engine->rootObjects().first()))
		{
			window->hide();
			window->setTransientParent(parentWindow);
			window->setModality(Qt::ApplicationModal);
			window->show();
		}

	while (!operation.isFinished())
	{
		QCoreApplication::processEvents();
		QThread::msleep(10);
	}

	request->accept();
}



void ServiceControl::textFeedback(const QString& title, const Operation& operation)
{
	printf("%s", qUtf8Printable(title));

	while (operation.isFinished() == false)
	{
		QCoreApplication::processEvents();
		QThread::msleep(200);
		printf(".");
	}
}
