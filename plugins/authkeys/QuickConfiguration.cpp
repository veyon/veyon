// SPDX-License-Identifier: GPL-2.0-or-later
#include "AuthKeysPlugin.h"
#include "QuickConfigurationController.h"

QObject* AuthKeysPlugin::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	return new AuthKeysQuickConfigurationController(static_cast<VeyonConfiguration*>(draft), parent);
}
