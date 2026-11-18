// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "QuickConfigurationPluginInterface.h"

#include <QFile>

#include "ConfigurationPagePluginInterface.h"
#include "FeatureProviderInterface.h"
#include "FileCollection.h"
#include "FileTransferConfiguration.h"

class FileCollectWorker;
class FileTransferController;

class FileTransferPlugin : public QObject, FeatureProviderInterface, PluginInterface, ConfigurationPagePluginInterface,
		public QuickConfigurationPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.FileTransfer")
	Q_INTERFACES(QuickConfigurationPluginInterface PluginInterface
					 FeatureProviderInterface
						 ConfigurationPagePluginInterface)
public:
	QObject* createQuickConfiguration(Configuration::Object* draft, QObject* parent) override;

	enum class FeatureCommand
	{
		StartFileTransfer,
		ContinueFileTransfer,
		CancelFileTransfer,
		FinishFileTransfer,
		OpenTransferFolder,
		StopWorker,
		InitFileCollection,
		FinishFileCollection,
		RetryFileTransfer,
		SkipFileTransfer,
	};
	Q_ENUM(FeatureCommand)

	enum class Argument {
		TransferId,
		FileName,
		DataChunk,
		OpenFileInApplication,
		OverwriteExistingFile,
		Files,
		CollectionId,
		FileSize,
		SourceDirectory,
		FilePattern,
		CollectRecursively,
		DestinationDirectory,
	};
	Q_ENUM(Argument)

	explicit FileTransferPlugin(QObject* parent = nullptr);
	~FileTransferPlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("d4bb9c42-9eef-4ecb-8dd5-dfd84b355481")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 0);
	}

	QString name() const override
	{
		return QStringLiteral("FileTransfer");
	}

	QString description() const override
	{
		return tr("Transfer files between computers");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	const FileTransferConfiguration& configuration() const
	{
		return m_configuration;
	}

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool startFeature(VeyonMasterInterface& master, const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	bool handleFeatureMessageFromWorker(VeyonServerInterface& server, const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

	void sendStartMessage(QUuid transferId, const QString& fileName,
						  bool overwriteExistingFile, const ComputerControlInterfaceList& interfaces,
						  const QString& destinationDirectory = {});
	void sendDataMessage(QUuid transferId, const QByteArray& data, const ComputerControlInterfaceList& interfaces);
	void sendCancelMessage(QUuid transferId, const ComputerControlInterfaceList& interfaces);
	void sendFinishMessage(QUuid transferId, const QString& fileName,
						   bool openFileInApplication, const ComputerControlInterfaceList& interfaces);
	void sendOpenTransferFolderMessage(const ComputerControlInterfaceList& interfaces);
	void sendStopWorkerMessage(const ComputerControlInterfaceList& interfaces);

	void sendInitFileCollectionMessage(const FileCollection& collection,
									   ComputerControlInterface::Pointer computerControlInterface);
	void sendInitFileCollectionMessage(const FileCollection& collection,
									   ComputerControlInterface::Pointer computerControlInterface,
									   const QString& sourceDirectory,
									   const QString& filePattern,
									   FileCollectController::SubfolderHandling subfolderHandling);
	void sendFinishFileCollectionMessage(const FileCollection& collection,
										 ComputerControlInterface::Pointer computerControlInterface);

	void sendStartFileTransferMessage(const FileCollection& collection,
									  ComputerControlInterface::Pointer computerControlInterface);
	void sendContinueFileTransferMessage(const FileCollection& collection,
										 ComputerControlInterface::Pointer computerControlInterface);

	ConfigurationPage* createConfigurationPage() override;

private:
	enum class LockedFileAction {
		RetryOpeningLockedFile,
		SkipLockedFile,
	};

	bool handleDistributeFilesMessage(const FeatureMessage& message);
	bool handleCollectFilesMessage(VeyonWorkerInterface& worker, const FeatureMessage& message);

	bool controlDistributeFilesFeature(Operation operation, const QVariantMap& arguments,
									   const ComputerControlInterfaceList& computerControlInterfaces);
	bool controlCollectFilesFeature(Operation operation, const QVariantMap& arguments,
									const ComputerControlInterfaceList& computerControlInterfaces);

	QString destinationDirectory() const;

	void processFileCollections();

	LockedFileAction queryLockedFileAction(const QString& fileName);

	const Feature m_distributeFilesFeature;
	const Feature m_collectFilesFeature;
	const FeatureList m_features = {m_distributeFilesFeature, m_collectFilesFeature};

	FileTransferConfiguration m_configuration;

	QString m_lastFileTransferSourceDirectory;

	FileTransferController* m_fileTransferController{nullptr};

	QFile m_currentFile{};
	QString m_currentFileName{};
	QUuid m_currentTransferId{};

	FileCollectController* m_fileCollectController = nullptr;

	QMap<FileCollection::Id, FileCollectWorker*> m_fileCollectWorkers;
	QMap<FileCollection::Id, MessageContext> m_fileCollectionContexts;
};
