// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QDir>

#include "PlatformServiceFunctions.h"
#include "VeyonServiceControl.h"


VeyonServiceControl::VeyonServiceControl(QWidget* parent) :
	ServiceControl(name(), filePath(), displayName(), parent)
{
}



bool VeyonServiceControl::setAutostart(bool enabled)
{
	return VeyonCore::platform().serviceFunctions().setStartMode(name(), enabled ? PlatformServiceFunctions::StartMode::Auto : PlatformServiceFunctions::StartMode::Manual);
}



QString VeyonServiceControl::name()
{
	return VeyonCore::platform().serviceFunctions().veyonServiceName();
}



QString VeyonServiceControl::filePath()
{
	return QDir::toNativeSeparators(
		QCoreApplication::applicationDirPath() +
		QDir::separator() +
		QStringLiteral("veyon-service") + VeyonCore::executableSuffix());
}



QString VeyonServiceControl::displayName()
{
	return tr("Veyon Service");
}
