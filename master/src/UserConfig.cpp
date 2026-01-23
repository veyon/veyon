/*
 * UserConfig.cpp - Configuration object storing personal settings
 *                  for the Veyon Master Application
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

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
