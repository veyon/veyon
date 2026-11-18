// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDir>

#include "Configuration/Property.h"
#include "Configuration/Proxy.h"
#include "QuickConfigurationPluginInterface.h"

#define FOREACH_FILE_TRANSFER_USER_CONFIG_PROPERTIES(OP) \
	OP(FileTransferUserConfiguration, config, QString, lastFileTransferSourceDirectory, setLastFileTransferSourceDirectory, "LastSourceDirectory", "FileTransfer", QDir::homePath(), Configuration::Property::Flag::Standard)

class FileTransferUserConfiguration : public Configuration::Proxy
{
public:
	explicit FileTransferUserConfiguration(Configuration::Object* object) :
		Configuration::Proxy(object)
	{
		QuickConfiguration::describe(lastFileTransferSourceDirectoryProperty(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Last source directory"), Configuration::Property::Editor::Directory);
	}

	FOREACH_FILE_TRANSFER_USER_CONFIG_PROPERTIES(DECLARE_CONFIG_PROPERTY)
};
