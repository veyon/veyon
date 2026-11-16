// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>

#include "PlatformServiceFunctions.h"
#include "ServiceControl.h"
#include "Toast.h"


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
	if (m_parent)
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
	auto toast = new Toast(m_parent);
	toast->setTitle(tr("Service control"));
	toast->setText(title);
	toast->applyPreset(Toast::Preset::Information);
	toast->setDuration(0);
	toast->show();

	while (operation.isRunning())
	{
		QCoreApplication::processEvents();
		QThread::msleep(10);
	}

	toast->hide();
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
