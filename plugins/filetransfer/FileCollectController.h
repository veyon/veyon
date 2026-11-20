// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ComputerControlInterface.h"
#include "FileCollection.h"

class FileTransferConfiguration;
class FileTransferPlugin;

// clazy:excludeall=ctor-missing-parent-argument
class FileCollectController : public QObject
{
	Q_OBJECT
public:
	enum class CollectingMode {
		CollectFilesFromSourceDirectory,
		PromptUserForFolder,
		PromptUserForFiles
	};
	Q_ENUM(CollectingMode)

	enum class CollectionDirectory {
		DestinationDirectory,
		DateTimeSubdirectory,
		PromptUserForName
	};
	Q_ENUM(CollectionDirectory)

	enum class CollectedFilesGroupingMode {
		None,
		PrefixFilenames,
		CreateSubdirectories
	};
	Q_ENUM(CollectedFilesGroupingMode)

	enum class CollectedFilesGroupingAttribute {
		None,
		UserLoginName,
		FullNameOfUser,
		FirstPartOfUserName,
		LastPartOfUserName,
		DeviceName,
	};
	Q_ENUM(CollectedFilesGroupingAttribute)

	enum class SubfolderHandling {
		Default,
		NonRecursive,
		Recursive,
	};
	Q_ENUM(SubfolderHandling)

	explicit FileCollectController(FileTransferPlugin* plugin);
	~FileCollectController() override;

	const FileTransferConfiguration& configuration() const;

	CollectionDirectory collectionDirectory() const
	{
		return m_collectionDirectory;
	}

	void setInterfaces(const ComputerControlInterfaceList& computerControlInterfaces);
	void setCollectionName(const QString& collectionName);

	void setCollectSourceDirectory(const QString& sourceDir);
	void setFilePattern(const QString& pattern);
	void setSubfolderHandling(SubfolderHandling subfolderHandling);
	void setDestinationDirectory(const QString& destDir);

	QString collectSourceDirectory() const { return m_collectSourceDirectory; }
	QString filePattern() const { return m_filePattern; }

	QString destinationDirectory() const
	{
		return m_destinationDirectory;
	}

	QString outputDirectory() const;

	QString outputFilePath(ComputerControlInterface::Pointer computerControlInterface,
						   const QString& fileName) const;

	// maps characters not allowed in file names to "_"
	static QString safeGroupingValue(QString value);

	void start();
	void stop();

	bool isRunning() const
	{
		return m_running;
	}

	int overallProgress() const;

	void initCollection(ComputerControlInterface::Pointer computerControlInterface,
						FileCollection::Id collectionId, const QStringList& files);
	void finishFileCollection(ComputerControlInterface::Pointer computerControlInterface,
							  FileCollection::Id collectionId);

	void startFileTransfer(ComputerControlInterface::Pointer computerControlInterface,
						   FileCollection::Id collectionId, FileCollection::TransferId transferId,
						   const QString& fileName, qint64 fileSize);
	void continueFileTransfer(ComputerControlInterface::Pointer computerControlInterface,
							  FileCollection::Id collectionId, FileCollection::TransferId transferId,
							  const QByteArray& dataChunk);
	void retryFileTransfer(ComputerControlInterface::Pointer computerControlInterface,
						   FileCollection::Id collectionId);
	void skipToNextFileTransfer(ComputerControlInterface::Pointer computerControlInterface,
								FileCollection::Id collectionId);
	void finishFileTransfer(ComputerControlInterface::Pointer computerControlInterface,
							FileCollection::Id collectionId, FileCollection::TransferId transferId);

	const auto& collections() const
	{
		return m_collections;
	}

Q_SIGNALS:
	void collectionsAboutToChange();
	void collectionsChanged();
	void collectionAboutToInitialize(const FileCollection::Id& fileCollection, int fileCount);
	void collectionInitialized(const FileCollection::Id& fileCollection);
	void collectionChanged(const FileCollection::Id& fileCollection);
	void overallProgressChanged();
	void started();
	void finished();

private:
	void initiateNextFileTransfer(ComputerControlInterface::Pointer computerControlInterface,
								  FileCollection& collection);

	FileTransferPlugin* m_plugin = nullptr;

	QMap<ComputerControlInterface::Pointer, FileCollection> m_collections{};

	QString m_destinationDirectory;
	QString m_collectionName;
	CollectionDirectory m_collectionDirectory;
	CollectedFilesGroupingMode m_collectedFilesGroupingMode;
	std::array<CollectedFilesGroupingAttribute, 3> m_collectedFilesGroupingAttributes;

	QString m_collectSourceDirectory;
	QString m_filePattern;
	SubfolderHandling m_subfolderHandling = SubfolderHandling::Default;

	bool m_running = false;
};
