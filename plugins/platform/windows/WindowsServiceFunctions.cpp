// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "WindowsServiceFunctions.h"
#include "WindowsServiceControl.h"
#include "WindowsServiceCore.h"


QString WindowsServiceFunctions::veyonServiceName() const
{
	return QStringLiteral("VeyonService");
}



bool WindowsServiceFunctions::isRegistered(const QString& name)
{
	return WindowsServiceControl(name).isRegistered();
}



bool WindowsServiceFunctions::isRunning(const QString& name)
{
	return WindowsServiceControl(name).isRunning();
}



bool WindowsServiceFunctions::start(const QString& name)
{
	return WindowsServiceControl(name).start();
}



bool WindowsServiceFunctions::stop(const QString& name)
{
	return WindowsServiceControl(name).stop();
}



bool WindowsServiceFunctions::install(const QString& name, const QString& filePath,
									  StartMode startMode, const QString& displayName)
{
	return WindowsServiceControl(name).install(filePath, displayName) &&
		setStartMode(name, startMode);
}



bool WindowsServiceFunctions::uninstall(const QString& name)
{
	return WindowsServiceControl(name).uninstall();
}



bool WindowsServiceFunctions::setStartMode(const QString& name, PlatformServiceFunctions::StartMode startMode)
{
	return WindowsServiceControl(name).setStartType(windowsServiceStartType(startMode));
}



bool WindowsServiceFunctions::runAsService(const QString& name, const ServiceEntryPoint& serviceEntryPoint)
{
	WindowsServiceCore windowsServiceCore(name, serviceEntryPoint);
	return windowsServiceCore.runAsService();
}



void WindowsServiceFunctions::manageServerInstances()
{
	WindowsServiceCore::instance()->manageServerInstances();
}



int WindowsServiceFunctions::windowsServiceStartType(PlatformServiceFunctions::StartMode startMode)
{
	switch (startMode)
	{
	case StartMode::Auto: return SERVICE_AUTO_START;
	case StartMode::Manual: return SERVICE_DEMAND_START;
	default: break;
	}

	return SERVICE_DISABLED;
}
