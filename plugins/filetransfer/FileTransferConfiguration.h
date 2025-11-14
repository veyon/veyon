/*
 * FileTransferConfiguration.h - configuration values for FileTransfer plugin
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include "VeyonConfiguration.h"
#include "Configuration/Proxy.h"
#include "FileCollectController.h"

#define FOREACH_FILE_TRANSFER_CONFIG_PROPERTY(OP) \
	OP(FileTransferConfiguration, m_configuration, bool, rememberLastFileTransferSourceDirectory, setRememberLastFileTransferSourceDirectory, "RememberLastSourceDirectory", "FileTransfer", true, Configuration::Property::Flag::Standard)	\
	OP(FileTransferConfiguration, m_configuration, bool, fileTransferCreateDestinationDirectory, setFileTransferCreateDestinationDirectory, "CreateDestinationDirectory", "FileTransfer", true, Configuration::Property::Flag::Standard)	\
	OP(FileTransferConfiguration, m_configuration, QString, fileTransferDefaultSourceDirectory, setFileTransferDefaultSourceDirectory, "DefaultSourceDirectory", "FileTransfer", QStringLiteral("%HOME%"), Configuration::Property::Flag::Standard)	\
	OP(FileTransferConfiguration, m_configuration, QString, fileTransferDestinationDirectory, setFileTransferDestinationDirectory, "DestinationDirectory", "FileTransfer", QStringLiteral("%HOME%"), Configuration::Property::Flag::Standard)	\
	OP(FileTransferConfiguration, m_configuration, QString, filesToCollectSourceDirectory, setFilesToCollectSourceDirectory, "FilesToCollectSourceDirectory", "FileTransfer", QStringLiteral("%DOCUMENTS%"), Configuration::Property::Flag::Standard)	\
	OP(FileTransferConfiguration, m_configuration, FileCollectController::CollectingMode, collectingMode, setCollectingMode, "CollectingMode", "FileTransfer", QVariant::fromValue(FileCollectController::CollectingMode::CollectFilesFromSourceDirectory), Configuration::Property::Flag::Standard)	\
	OP(FileTransferConfiguration, m_configuration, bool, collectFilesRecursively, setCollectFilesRecursively, "CollectFilesRecursively", "FileTransfer", false, Configuration::Property::Flag::Standard)	\
	OP(FileTransferConfiguration, m_configuration, QString, collectedFilesDestinationDirectory, setCollectedFilesDestinationDirectory, "CollectedFilesDestinationDirectory", "FileTransfer", QStringLiteral("%HOME%"), Configuration::Property::Flag::Standard)	\
	OP(FileTransferConfiguration, m_configuration, FileCollectController::CollectionDirectory, collectionDirectory, setCollectionDirectory, "CollectionDirectory", "FileTransfer", QVariant::fromValue(FileCollectController::CollectionDirectory::DestinationDirectory), Configuration::Property::Flag::Standard)	\
	OP(FileTransferConfiguration, m_configuration, FileCollectController::CollectedFilesGroupingMode, collectedFilesGroupingMode, setCollectedFilesGroupingMode, "CollectedFilesGroupingMode", "FileTransfer", QVariant::fromValue(FileCollectController::CollectedFilesGroupingMode::CreateSubdirectories), Configuration::Property::Flag::Standard)	\
	OP(FileTransferConfiguration, m_configuration, FileCollectController::CollectedFilesGroupingAttribute, collectedFilesGroupingAttribute, setCollectedFilesGroupingAttribute, "CollectedFilesGroupingAttibute", "FileTransfer", QVariant::fromValue(FileCollectController::CollectedFilesGroupingAttribute::UserLoginName), Configuration::Property::Flag::Standard)	\

// clazy:excludeall=missing-qobject-macro

DECLARE_CONFIG_PROXY(FileTransferConfiguration, FOREACH_FILE_TRANSFER_CONFIG_PROPERTY)
