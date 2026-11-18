// SPDX-License-Identifier: GPL-2.0-or-later
#include "FileTransferPlugin.h"
#include "FileTransferConfiguration.h"
#include "QuickConfigurationPluginInterface.h"
#include "VeyonConfiguration.h"

QObject* FileTransferPlugin::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	auto config = new FileTransferConfiguration(draft);
	config->setParent(parent);
	using namespace QuickConfiguration;
	using Editor = Property::Editor;
	describe(config->rememberLastFileTransferSourceDirectoryProperty(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Remember the last source directory"), Editor::Automatic, {}, {}, {});
	describe(config->fileTransferCreateDestinationDirectoryProperty(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Create destination directory"), Editor::Automatic, {}, {}, {});
	describe(config->fileTransferDefaultSourceDirectoryProperty(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Default source directory"), Editor::Directory, {}, {}, {});
	describe(config->fileTransferDestinationDirectoryProperty(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Destination directory"), Editor::Directory, {}, {}, {});
	describe(config->filesToCollectSourceDirectoryProperty(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Directory to collect files from"), Editor::Directory, {}, {}, {});
	describe(config->collectingModeProperty(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Collection mode"), Editor::Automatic, {}, {}, {});
	describe(config->filesToExcludeFromCollectingProperty(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Files to exclude"), Editor::Automatic, {}, {}, {});
	describe(config->collectFilesRecursivelyProperty(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Collect files from subdirectories"), Editor::Automatic, {}, {}, {});
	describe(config->collectedFilesDestinationDirectoryProperty(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Collected files destination"), Editor::Directory, {}, {}, {});
	describe(config->collectionDirectoryProperty(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Collection directory"), Editor::Automatic, {}, {}, {});
	describe(config->collectedFilesGroupingModeProperty(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Group collected files"), Editor::Automatic, {}, {}, {});
	describe(config->collectedFilesGroupingAttribute1Property(), QStringLiteral("filetransfer"), tr("File transfer"), tr("First grouping attribute"), Editor::Automatic, {}, {}, {});
	describe(config->collectedFilesGroupingAttribute2Property(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Second grouping attribute"), Editor::Automatic, {}, {}, {});
	describe(config->collectedFilesGroupingAttribute3Property(), QStringLiteral("filetransfer"), tr("File transfer"), tr("Third grouping attribute"), Editor::Automatic, {}, {}, {});
	choices(config->collectingModeProperty(), {{0, tr("Collect from source directory")}, {1, tr("Ask user to select a folder")}, {2, tr("Ask user to select files")}});
	choices(config->collectionDirectoryProperty(), {{0, tr("Destination directory")}, {1, tr("Date and time subdirectory")}, {2, tr("Ask for collection name")}});
	choices(config->collectedFilesGroupingModeProperty(), {{0, tr("Do not group files")}, {1, tr("Prefix filenames")}, {2, tr("Create subdirectories")}});
	choices(config->collectedFilesGroupingAttribute1Property(), {{0, tr("None")}, {1, tr("User login name")}, {2, tr("Full user name")}, {3, tr("First part of user name")}, {4, tr("Last part of user name")}, {5, tr("Computer name")}});
	choices(config->collectedFilesGroupingAttribute2Property(), {{0, tr("None")}, {1, tr("User login name")}, {2, tr("Full user name")}, {3, tr("First part of user name")}, {4, tr("Last part of user name")}, {5, tr("Computer name")}});
	choices(config->collectedFilesGroupingAttribute3Property(), {{0, tr("None")}, {1, tr("User login name")}, {2, tr("Full user name")}, {3, tr("First part of user name")}, {4, tr("Last part of user name")}, {5, tr("Computer name")}});
	depends(config->fileTransferDefaultSourceDirectoryProperty(), {QStringLiteral("FileTransfer/RememberLastSourceDirectory")}, [config]() { return !config->rememberLastFileTransferSourceDirectory(); });
	depends(config->collectedFilesGroupingAttribute1Property(), {QStringLiteral("FileTransfer/CollectedFilesGroupingMode")}, [config]() { return config->collectedFilesGroupingMode() != FileCollectController::CollectedFilesGroupingMode::None; });
	depends(config->collectedFilesGroupingAttribute2Property(), {QStringLiteral("FileTransfer/CollectedFilesGroupingMode")}, [config]() { return config->collectedFilesGroupingMode() != FileCollectController::CollectedFilesGroupingMode::None; });
	depends(config->collectedFilesGroupingAttribute3Property(), {QStringLiteral("FileTransfer/CollectedFilesGroupingMode")}, [config]() { return config->collectedFilesGroupingMode() != FileCollectController::CollectedFilesGroupingMode::None; });
	config->setObjectName(QStringLiteral("filetransfer"));
	return config;
}
