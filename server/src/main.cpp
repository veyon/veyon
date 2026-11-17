// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QGuiApplication>

#include "ComputerControlServer.h"
#include "IpcEndpointCredentials.h"
#include "Logger.h"
#include "ProcessHardening.h"

// veyon-service writes the IPC credentials right after starting the server
static constexpr auto ServiceIpcCredentialsTimeout = 5000;


int main(int argc, char** argv)
{
	// first statement: protect IPC keys, TLS host key and logon credentials (02-ipc.md §1a)
	if (ProcessHardening::hardenCurrentProcess() == false)
	{
		return -1;
	}

	// read the IPC credentials from stdin before any other work (02-ipc.md §2)
	auto serviceIpc = IpcEndpointCredentials::readFromStandardInput(ServiceIpcCredentialsTimeout);
	if (serviceIpc.result == IpcStandardInputCredentials::Result::Invalid)
	{
		// no logger exists yet
		Logger::logEarlyCritical("veyon-server: invalid or incomplete IPC credentials on stdin - exiting");
		return -1;
	}

	VeyonCore::setupApplicationParameters();

	// before constructing QGuiApplication, see VeyonCaptureHelper
	QGuiApplication::setDesktopFileName(QStringLiteral("io.veyon.veyon-server"));
	QGuiApplication app(argc, argv);

	VeyonCore core(&app, VeyonCore::Component::Server, QStringLiteral("Server"),
				   VeyonCore::defaultInitialization() | VeyonCore::InitializationFlag::ServiceIpc);

	if (serviceIpc.result == IpcStandardInputCredentials::Result::Valid)
	{
		if (core.connectToServiceIpc(std::move(*serviceIpc.credentials)) == false)
		{
			// fail closed: the service restarts the server with a new endpoint
			vCritical() << "Failed to connect to service";
			return -1;
		}
	}
	else
	{
		vInfo() << "no IPC credentials on stdin - running without service connection";
	}

	ComputerControlServer server(&core);
	if (server.start() == false)
	{
		vCritical() << "Failed to start server";
		return -1;
	}

	return core.exec();
}
