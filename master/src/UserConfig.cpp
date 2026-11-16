// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/JsonStore.h"
#include "Filesystem.h"
#include "UserConfig.h"
#include "VeyonConfiguration.h"


UserConfig::UserConfig(const QString& storeName) :
	Configuration::Object(Configuration::Store::Backend::JsonFile, Configuration::Store::Scope::User, storeName)
{
	const QString templateFileName = VeyonCore::filesystem().expandPath(VeyonCore::config().configurationTemplatesDirectory()) +
		QDir::separator() + storeName + QStringLiteral(".json");
	if (QFileInfo(templateFileName).isReadable())
	{
		Configuration::JsonStore jsonStore(Configuration::Store::Scope::System, templateFileName);
		*this += UserConfig(&jsonStore);
	}
}



UserConfig::UserConfig(Configuration::Store* store) :
	Configuration::Object(store)
{
}
