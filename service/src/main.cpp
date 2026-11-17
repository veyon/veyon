// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>

#include "PlatformServiceFunctions.h"
#include "ProcessHardening.h"
#include "VeyonServiceControl.h"

int main(int argc, char** argv)
{
	// no core dumps of the service holding secrets, also without systemd's LimitCORE=0 (02-ipc.md §1a.6);
	// the servers and workers inherit the limit and set it themselves as well
	if (ProcessHardening::disableCoreDumps() == false)
	{
		qWarning("veyon-service: failed to disable core dumps");
	}

	VeyonCore::setupApplicationParameters();

	QCoreApplication app(argc, argv);
	VeyonCore core(&app, VeyonCore::Component::Service, QStringLiteral("Service"));

	auto& serviceFunctions = core.platform().serviceFunctions();

	if (serviceFunctions.runAsService(VeyonServiceControl::name(),
									  [&]() { serviceFunctions.manageServerInstances(); }))
	{
		return 0;
	}

	return -1;
}
