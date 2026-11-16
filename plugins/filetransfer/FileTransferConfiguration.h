// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Proxy.h"
#include "FileCollectController.h"
#include "VeyonConfiguration.h"

#define FOREACH_FILE_TRANSFER_CONFIG_PROPERTY(OP) \
	OP(FileTransferConfiguration, m_configuration, bool, rememberLastFileTransferSourceDirectory, setRememberLastFileTransferSourceDirectory, "RememberLastSourceDirectory", "FileTransfer", true, Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, bool, fileTransferCreateDestinationDirectory, setFileTransferCreateDestinationDirectory, "CreateDestinationDirectory", "FileTransfer", true, Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, bool, allowCustomDestinationDirectory, setAllowCustomDestinationDirectory, "AllowCustomDestinationDirectory", "FileTransfer", true, Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, QString, fileTransferDefaultSourceDirectory, setFileTransferDefaultSourceDirectory, "DefaultSourceDirectory", "FileTransfer", QStringLiteral("%HOME%"), Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, QString, fileTransferDestinationDirectory, setFileTransferDestinationDirectory, "DestinationDirectory", "FileTransfer", QStringLiteral("%HOME%"), Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, QString, filesToCollectSourceDirectory, setFilesToCollectSourceDirectory, "FilesToCollectSourceDirectory", "FileTransfer", QStringLiteral("%DOCUMENTS%"), Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, FileCollectController::CollectingMode, collectingMode, setCollectingMode, "CollectingMode", "FileTransfer", QVariant::fromValue(FileCollectController::CollectingMode::CollectFilesFromSourceDirectory), Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, QString, filesToExcludeFromCollecting, setFilesToExcludeFromCollecting, "FilesToExcludeFromCollecting", "FileTransfer", QStringLiteral("*.lnk;*.desktop"), Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, bool, collectFilesRecursively, setCollectFilesRecursively, "CollectFilesRecursively", "FileTransfer", false, Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, QString, collectedFilesDestinationDirectory, setCollectedFilesDestinationDirectory, "CollectedFilesDestinationDirectory", "FileTransfer", QStringLiteral("%HOME%"), Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, FileCollectController::CollectionDirectory, collectionDirectory, setCollectionDirectory, "CollectionDirectory", "FileTransfer", QVariant::fromValue(FileCollectController::CollectionDirectory::DestinationDirectory), Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, FileCollectController::CollectedFilesGroupingMode, collectedFilesGroupingMode, setCollectedFilesGroupingMode, "CollectedFilesGroupingMode", "FileTransfer", QVariant::fromValue(FileCollectController::CollectedFilesGroupingMode::CreateSubdirectories), Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, FileCollectController::CollectedFilesGroupingAttribute, collectedFilesGroupingAttribute1, setCollectedFilesGroupingAttribute1, "CollectedFilesGroupingAttibute1", "FileTransfer", QVariant::fromValue(FileCollectController::CollectedFilesGroupingAttribute::UserLoginName), Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, FileCollectController::CollectedFilesGroupingAttribute, collectedFilesGroupingAttribute2, setCollectedFilesGroupingAttribute2, "CollectedFilesGroupingAttibute2", "FileTransfer", QVariant::fromValue(FileCollectController::CollectedFilesGroupingAttribute::None), Configuration::Property::Flag::Standard) \
	OP(FileTransferConfiguration, m_configuration, FileCollectController::CollectedFilesGroupingAttribute, collectedFilesGroupingAttribute3, setCollectedFilesGroupingAttribute3, "CollectedFilesGroupingAttibute3", "FileTransfer", QVariant::fromValue(FileCollectController::CollectedFilesGroupingAttribute::None), Configuration::Property::Flag::Standard)

// clazy:excludeall=missing-qobject-macro

DECLARE_CONFIG_PROXY(FileTransferConfiguration, FOREACH_FILE_TRANSFER_CONFIG_PROPERTY)
