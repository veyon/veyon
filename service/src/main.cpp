// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>

#include "PlatformServiceFunctions.h"
#include "VeyonServiceControl.h"

int main(int argc, char** argv)
{
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
