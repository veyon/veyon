// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CommandLineIO.h"
#include "ServiceControlCommands.h"
#include "VeyonServiceControl.h"

ServiceControlCommands::ServiceControlCommands(QObject* parent) :
	QObject(parent),
	m_commands({
		{QStringLiteral("register"), tr("Register Veyon Service")},
		{QStringLiteral("unregister"), tr("Unregister Veyon Service")},
		{QStringLiteral("start"), tr("Start Veyon Service")},
		{QStringLiteral("stop"), tr("Stop Veyon Service")},
		{QStringLiteral("restart"), tr("Restart Veyon Service")},
		{QStringLiteral("status"), tr("Query status of Veyon Service")},
	})
{
}



CommandLinePluginInterface::RunResult ServiceControlCommands::handle_register(const QStringList& arguments)
{
	Q_UNUSED(arguments)

	VeyonServiceControl serviceControl;
	serviceControl.registerService();

	return serviceControl.isServiceRegistered() ? Successful : Failed;
}



CommandLinePluginInterface::RunResult ServiceControlCommands::handle_unregister(const QStringList& arguments)
{
	Q_UNUSED(arguments)
	VeyonServiceControl serviceControl;
	serviceControl.unregisterService();

	return serviceControl.isServiceRegistered() ? Failed : Successful;
}



CommandLinePluginInterface::RunResult ServiceControlCommands::handle_start(const QStringList& arguments)
{
	Q_UNUSED(arguments)

	VeyonServiceControl serviceControl;
	serviceControl.startService();

	return serviceControl.isServiceRunning() ? Successful : Failed;
}



CommandLinePluginInterface::RunResult ServiceControlCommands::handle_stop(const QStringList& arguments)
{
	Q_UNUSED(arguments)

	VeyonServiceControl serviceControl;
	serviceControl.stopService();

	return serviceControl.isServiceRunning() ? Failed : Successful;
}



CommandLinePluginInterface::RunResult ServiceControlCommands::handle_restart(const QStringList& arguments)
{
	handle_stop(arguments);
	return handle_start(arguments);
}



CommandLinePluginInterface::RunResult ServiceControlCommands::handle_status(const QStringList& arguments)
{
	Q_UNUSED(arguments)

	if (VeyonServiceControl().isServiceRunning())
	{
		CommandLineIO::print(tr("Service is running"));
	}
	else
	{
		CommandLineIO::print(tr("Service is not running"));
	}

	return NoResult;
}
