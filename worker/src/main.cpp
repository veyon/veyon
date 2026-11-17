// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QIcon>

#include "Feature.h"
#include "IpcEndpointCredentials.h"
#include "Logger.h"
#include "ProcessHardening.h"
#include "VeyonWorker.h"


// the server writes the IPC credentials right after starting the worker
static constexpr auto WorkerIpcCredentialsTimeout = 5000;


int main(int argc, char** argv)
{
	// first statement: workers may hold an ephemeral demo key and receive feature data that must
	// not be readable by other processes of the same uid (02-ipc.md §1a)
	if (ProcessHardening::hardenCurrentProcess() == false)
	{
		return -1;
	}

	// read the IPC credentials from stdin before any other work (02-ipc.md §2, §6a); a worker is
	// always started by the server, so it can't run without them
	auto workerIpc = IpcEndpointCredentials::readFromStandardInput(WorkerIpcCredentialsTimeout);
	if (workerIpc.result != IpcStandardInputCredentials::Result::Valid)
	{
		Logger::logEarlyCritical("veyon-worker: missing or invalid IPC credentials on stdin - exiting");
		return -1;
	}

	VeyonCore::setupApplicationParameters();

	QApplication app(argc, argv);
	app.setWindowIcon(QIcon(QStringLiteral(":/core/icon64.png")));

	const auto arguments = app.arguments();

	if (arguments.count() < 2)
	{
		qFatal("Not enough arguments (feature)");
	}

	const auto featureUid = Feature::Uid{arguments[1]};
	if (featureUid.isNull())
	{
		qFatal("Invalid feature UID given");
	}

	// Linux: directory of the endpoint (/run/veyon or the server's $XDG_RUNTIME_DIR/veyon)
	const auto endpointDirectory = arguments.value(2);
#ifndef Q_OS_WIN
	if (endpointDirectory.startsWith(QLatin1Char('/')) == false)
	{
		qFatal("Invalid endpoint directory given");
	}
#endif

	VeyonWorker worker(featureUid, endpointDirectory, std::move(*workerIpc.credentials));

	return worker.core().exec();
}
