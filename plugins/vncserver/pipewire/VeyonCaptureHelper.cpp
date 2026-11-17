// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

// veyon-capture-helper (02-ipc.md §1a.5): runs every xdg-desktop-portal call (ScreenCast,
// RemoteDesktop input) and the PipeWire stream for the non-dumpable Wayland veyon-server, which
// the portal would reject (spike S7), and serves the screen via RFB on a Unix domain socket in the
// private endpoint directory. Started by veyon-server only; it stays dumpable and therefore
// receives nothing but its per-start VNC password and socket name. A process of the session user
// can kill, trace or replace it (residual risk R-19).

#include <QGuiApplication>

#include "CaptureHelperCredentials.h"
#include "IpcServer.h"
#include "Logger.h"
#include "PipeWireCaptureServer.h"
#include "PipeWireVncConfiguration.h"
#include "ProcessHardening.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"

#include <unistd.h>


int main(int argc, char** argv)
{
	// no prctl(PR_SET_DUMPABLE, 0) here: the portal has to open /proc/<pid>/root of its caller
	if (ProcessHardening::disableCoreDumps() == false)
	{
		Logger::logEarlyCritical("veyon-capture-helper: failed to disable core dumps - exiting");
		return -1;
	}

	// the helper's memory is readable by every process of its user: never run it as root (debug
	// launch paths of veyon-service start the server and thus the helper as root)
	if (getuid() == 0 || geteuid() == 0)
	{
		Logger::logEarlyCritical("veyon-capture-helper: refusing to run as root - exiting");
		return -1;
	}

	// read the VNC password from stdin before any other work, like veyon-server does
	auto credentials = CaptureHelperCredentials::readFromStandardInput(CaptureHelperCredentials::StandardInputTimeout);
	if (credentials.has_value() == false)
	{
		Logger::logEarlyCritical("veyon-capture-helper: missing or invalid credentials on stdin - exiting");
		return -1;
	}

	VeyonCore::setupApplicationParameters();

	// the portal permission store and the KDE pre-authorization use the app ID of the server; set it
	// before constructing QGuiApplication since Qt >= 6.10 registers it with xdg-desktop-portal >= 1.20
	// right away and the portal refuses changing it afterwards
	QGuiApplication::setDesktopFileName(QStringLiteral("io.veyon.veyon-server"));
	QGuiApplication app(argc, argv);

	// dumpable: platform plugin, read-only configuration, logger and session ID only; neither
	// service IPC nor SecretStore
	VeyonCore core(&app, VeyonCore::Component::CaptureHelper, QStringLiteral("CaptureHelper"),
				   VeyonCore::minimalInitialization());

	// the same directory veyon-server created (0700, owned by the session user)
	const auto socketDirectory = IpcServer::workerEndpointDirectory();
	if (socketDirectory.isEmpty() || IpcServer::prepareDirectory(socketDirectory, IpcServer::DirectoryType::Private) == false)
	{
		vCritical() << "no valid socket directory";
		return -1;
	}

	const PipeWireVncConfiguration configuration(&VeyonCore::config());

	PipeWireCaptureServer captureServer(configuration.persistRestoreToken());
	if (captureServer.run(socketDirectory + QLatin1Char('/') + credentials->socketName(), credentials->password()) == false)
	{
		vCritical() << "failed to run capture server";
		return -1;
	}

	return 0;
}
