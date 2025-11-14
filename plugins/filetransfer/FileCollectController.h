/*
 * FileCollectController.h - declaration of FileCollectController class
 *
 * Copyright (c) 2025 Tobias Junghans <tobydox@veyon.io>
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

#include "ComputerControlInterface.h"
#include "FileCollection.h"

class FileTransferConfiguration;
class FileTransferPlugin;

// clazy:excludeall=ctor-missing-parent-argument
class FileCollectController : public QObject
{
	Q_OBJECT
public:
	enum CollectingMode {
		CollectFilesFromSourceDirectory,
		PromptUserForFolder,
		PromptUserForFiles
	};
	Q_ENUM(CollectingMode)

	enum CollectionDirectory {
		DestinationDirectory,
		DateTimeSubdirectory,
		PromptUserForName
	};
	Q_ENUM(CollectionDirectory)

	enum CollectedFilesGroupingMode {
		None,
		PrefixFilenames,
		CreateSubdirectories
	};
	Q_ENUM(CollectedFilesGroupingMode)

	enum CollectedFilesGroupingAttribute {
		UserLoginName,
		FullNameOfUser,
		DeviceName
	};
	Q_ENUM(CollectedFilesGroupingAttribute)

	explicit FileCollectController(FileTransferPlugin* plugin);
	~FileCollectController() override;

	const FileTransferConfiguration& configuration() const;

	CollectionDirectory collectionDirectory() const
	{
		return m_collectionDirectory;
	}

	CollectedFilesGroupingMode collectedFilesGroupingMode() const
	{
		return m_collectedFilesGroupingMode;
	}

	CollectedFilesGroupingAttribute collectedFilesGroupingAttribute() const
	{
		return m_collectedFilesGroupingAttribute;
	}

	void setInterfaces(const ComputerControlInterfaceList& computerControlInterfaces);
	void setCollectionName(const QString& collectionName);

	QString outputDirectory() const;

	QString outputFilePath(ComputerControlInterface::Pointer computerControlInterface,
						   const QString& fileName) const;

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
	CollectedFilesGroupingAttribute m_collectedFilesGroupingAttribute;

	bool m_running = false;

};
