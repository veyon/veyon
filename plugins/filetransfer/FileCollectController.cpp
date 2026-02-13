/*
 * FileCollectController.cpp - implementation of FileCollectController class
 *
 * Copyright (c) 2025-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QFileInfo>

#include "FileCollectController.h"
#include "FileTransferPlugin.h"
#include "Filesystem.h"

FileCollectController::FileCollectController(FileTransferPlugin* plugin) :
	QObject(plugin),
	m_plugin(plugin),
	m_destinationDirectory(VeyonCore::filesystem().expandPath(plugin->configuration().collectedFilesDestinationDirectory())),
	m_collectionDirectory(configuration().collectionDirectory()),
	m_collectedFilesGroupingMode(configuration().collectedFilesGroupingMode()),
	m_collectedFilesGroupingAttributes({configuration().collectedFilesGroupingAttribute1(),
									   configuration().collectedFilesGroupingAttribute2(),
									   configuration().collectedFilesGroupingAttribute3()})
{
	connect (this, &FileCollectController::collectionChanged, this, &FileCollectController::overallProgressChanged);
}



FileCollectController::~FileCollectController()
{
}



const FileTransferConfiguration& FileCollectController::configuration() const
{
	return m_plugin->configuration();
}



void FileCollectController::setInterfaces(const ComputerControlInterfaceList& computerControlInterfaces)
{
	if (isRunning() == false)
	{
		Q_EMIT collectionsAboutToChange();

		m_collections.clear();

		for (const auto& computerControlInterface : computerControlInterfaces)
		{
			if (computerControlInterface->state() == ComputerControlInterface::State::Connected)
			{
				FileCollection collection{};
				collection.state = FileCollection::State::Initializing;
				for (const auto& name : {computerControlInterface->userFullName(),
					 computerControlInterface->userLoginName(),
					 computerControlInterface->computerName()})
				{
					if (name.isEmpty() == false)
					{
						collection.name = name;
						break;
					}
				}
				m_collections[computerControlInterface] = collection;
			}
		}

		Q_EMIT collectionsChanged();
	}
}



void FileCollectController::setCollectionName(const QString& collectionName)
{
	m_collectionName = collectionName;
}



QString FileCollectController::outputDirectory() const
{
	if (m_collectionName.isEmpty())
	{
		return m_destinationDirectory;
	}

	return m_destinationDirectory + QDir::separator() + m_collectionName;
}



QString FileCollectController::outputFilePath(ComputerControlInterface::Pointer computerControlInterface,
											  const QString& fileName) const
{
	const QString defaultFilePath = outputDirectory() + QDir::separator() + fileName;
	if (m_collectedFilesGroupingMode == CollectedFilesGroupingMode::None)
	{
		return defaultFilePath;
	}

	const auto groupingValues = std::accumulate(m_collectedFilesGroupingAttributes.cbegin(),
												m_collectedFilesGroupingAttributes.cend(),
												QStringList{},
												[=](QStringList attributes, CollectedFilesGroupingAttribute attribute) {
		const auto userName = VeyonCore::stripDomain(computerControlInterface->userFullName());
		const auto userNameSplitted = userName.split(QLatin1Char(' '), Qt::SkipEmptyParts);
		switch (attribute)
		{
		case CollectedFilesGroupingAttribute::None:
			break;
		case CollectedFilesGroupingAttribute::UserLoginName:
			attributes += VeyonCore::stripDomain(computerControlInterface->userLoginName());
			break;
		case CollectedFilesGroupingAttribute::FullNameOfUser:
			attributes += userName;
			break;
		case CollectedFilesGroupingAttribute::FirstPartOfUserName:
			if (userNameSplitted.isEmpty() == false)
			{
				attributes += userNameSplitted.mid(0, userNameSplitted.size() - 1).join(QLatin1Char(' '));
			}
			break;
		case CollectedFilesGroupingAttribute::LastPartOfUserName:
			if (userNameSplitted.isEmpty() == false)
			{
				attributes += userNameSplitted.constLast();
			}
			break;
		case CollectedFilesGroupingAttribute::DeviceName:
			attributes += computerControlInterface->computerName();
			break;
		}
		return attributes;
	});

	static const QRegularExpression invalidFileNameCharacters(QStringLiteral("[\x00-\x1F<>:\"/\\|?*\x7F]"));
	const auto groupingValue = groupingValues.join(QLatin1Char('_')).replace(invalidFileNameCharacters, QString{});
	if (groupingValue.isEmpty())
	{
		return defaultFilePath;
	}

	switch (m_collectedFilesGroupingMode)
	{
	case CollectedFilesGroupingMode::CreateSubdirectories:
		return outputDirectory() + QDir::separator() + groupingValue + QDir::separator() + fileName;
	case CollectedFilesGroupingMode::PrefixFilenames:
		return outputDirectory() + QDir::separator() + groupingValue + QStringLiteral("_") + fileName;
	case CollectedFilesGroupingMode::None:
		break;
	}

	return defaultFilePath;
}



void FileCollectController::start()
{
	if (isRunning() == false && m_collections.isEmpty() == false)
	{
		for (auto it = m_collections.constBegin(), end = m_collections.constEnd(); it != end; ++it)
		{
			m_plugin->sendInitFileCollectionMessage(it.value(), it.key());
		}

		m_running = true;

		Q_EMIT started();
	}
}



void FileCollectController::stop()
{
	if (isRunning())
	{
		m_running = false;

		Q_EMIT finished();
		Q_EMIT overallProgressChanged();

		for (auto it = m_collections.constBegin(), end = m_collections.constEnd(); it != end; ++it)
		{
			m_plugin->sendFinishFileCollectionMessage(it.value(), it.key());
		}
	}
}



int FileCollectController::overallProgress() const
{
	return m_collections.isEmpty() ?
				0
			  :
				std::accumulate(
					m_collections.constBegin(),
					m_collections.constEnd(),
					0,
					[](int sum, const FileCollection& collection) -> int {
		return sum + collection.progress();
	}) / m_collections.size();
}



void FileCollectController::initCollection(ComputerControlInterface::Pointer computerControlInterface,
										   FileCollection::Id collectionId, const QStringList& files)
{
	if (m_collections.value(computerControlInterface).id == collectionId)
	{
		Q_EMIT collectionAboutToInitialize(collectionId, files.size());

		auto& collection = m_collections[computerControlInterface]; // clazy:exclude=detaching-member
		collection.files = files;

		Q_EMIT collectionInitialized(collectionId);

		initiateNextFileTransfer(computerControlInterface, collection);
	}
	else
	{
		vWarning() << "collection" << collectionId << "not found";
	}
}



void FileCollectController::finishFileCollection(ComputerControlInterface::Pointer computerControlInterface, FileCollection::Id collectionId)
{
	if (m_collections.value(computerControlInterface).id == collectionId)
	{
		auto& collection = m_collections[computerControlInterface]; // clazy:exclude=detaching-member
		collection.state = FileCollection::State::Finished;

		Q_EMIT collectionChanged(collection.id);

		bool allFinished = true;
		for (const auto& collection : std::as_const(m_collections))
		{
			if (collection.state != FileCollection::State::Finished)
			{
				allFinished = false;
				break;
			}
		}

		if (allFinished)
		{
			stop();
		}
	}
	else
	{
		vWarning() << "collection" << collectionId << "not found";
	}
}



void FileCollectController::startFileTransfer(ComputerControlInterface::Pointer computerControlInterface,
											  FileCollection::Id collectionId, FileCollection::TransferId transferId,
											  const QString& fileName, qint64 fileSize)
{
	if (m_collections.value(computerControlInterface).id == collectionId)
	{
		auto& collection = m_collections[computerControlInterface]; // clazy:exclude=detaching-member
		if (collection.currentOutputFile)
		{
			vCritical() << "previous file transfer not finished";
			delete collection.currentOutputFile;
			collection.currentOutputFile = nullptr;
		}

		collection.currentOutputFile = new QFile(outputFilePath(computerControlInterface, fileName));
		const auto effectiveOutputPath = QFileInfo(collection.currentOutputFile->fileName()).absolutePath();
		if (VeyonCore::filesystem().ensurePathExists(effectiveOutputPath) == false)
		{
			vCritical() << "failed to create directory" << effectiveOutputPath;
			delete collection.currentOutputFile;
			collection.currentOutputFile = nullptr;
			return;
		}

		if (collection.currentOutputFile->open(QFile::WriteOnly | QFile::Truncate) == false)
		{
			vCritical() << "failed to open" << collection.currentOutputFile->fileName() << "for writing";
			delete collection.currentOutputFile;
			collection.currentOutputFile = nullptr;
			return;
		}

		collection.currentTransferId = transferId;
		collection.currentFileSize = fileSize;
		collection.state = FileCollection::State::FileTransferRunning;

		Q_EMIT collectionChanged(collectionId);

		m_plugin->sendContinueFileTransferMessage(collection, computerControlInterface);
	}
	else
	{
		vWarning() << "collection" << collectionId << "not found";
	}
}



void FileCollectController::continueFileTransfer(ComputerControlInterface::Pointer computerControlInterface,
												 FileCollection::Id collectionId, FileCollection::TransferId transferId,
												 const QByteArray& dataChunk)
{
	if (m_collections.value(computerControlInterface).id == collectionId)
	{
		auto& collection = m_collections[computerControlInterface]; // clazy:exclude=detaching-member
		if (collection.currentTransferId != transferId)
		{
			vCritical() << "file transfer ID does not match" << collection.currentTransferId << transferId;
			return;
		}
		if (collection.state == FileCollection::State::Finished)
		{
			vDebug() << "file collection canceled or finished";
			return;
		}

		if (collection.currentOutputFile)
		{
			collection.currentOutputFile->write(dataChunk);
		}
		else
		{
			vCritical() << "transfer not started / output file not available" << transferId;
		}

		Q_EMIT collectionChanged(collectionId);

		m_plugin->sendContinueFileTransferMessage(collection, computerControlInterface);
	}
	else
	{
		vWarning() << "collection" << collectionId << "not found";
	}
}



void FileCollectController::retryFileTransfer(ComputerControlInterface::Pointer computerControlInterface,
											  FileCollection::Id collectionId)
{
	if (m_collections.value(computerControlInterface).id == collectionId)
	{
		auto& collection = m_collections[computerControlInterface]; // clazy:exclude=detaching-member

		m_plugin->sendStartFileTransferMessage(collection, computerControlInterface);
	}
	else
	{
		vWarning() << "collection" << collectionId << "not found";
	}
}



void FileCollectController::skipToNextFileTransfer(ComputerControlInterface::Pointer computerControlInterface,
												   FileCollection::Id collectionId)
{
	if (m_collections.value(computerControlInterface).id == collectionId)
	{
		auto& collection = m_collections[computerControlInterface]; // clazy:exclude=detaching-member

		collection.processedFilesCount += 1;

		initiateNextFileTransfer(computerControlInterface, collection);
	}
	else
	{
		vWarning() << "collection" << collectionId << "not found";
	}
}



void FileCollectController::finishFileTransfer(ComputerControlInterface::Pointer computerControlInterface,
											   FileCollection::Id collectionId, FileCollection::TransferId transferId)
{
	if (m_collections.value(computerControlInterface).id == collectionId)
	{
		auto& collection = m_collections[computerControlInterface]; // clazy:exclude=detaching-member

		if (collection.currentTransferId != transferId)
		{
			vCritical() << "file transfer ID does not match" << collection.currentTransferId << transferId;
			return;
		}

		if (collection.currentOutputFile)
		{
			collection.currentOutputFile->close();
			delete collection.currentOutputFile;
			collection.currentOutputFile = nullptr;
		}
		else
		{
			vCritical() << "transfer not started / output file not available" << transferId;
		}

		collection.currentFileSize = 0;
		collection.currentTransferId = FileCollection::TransferId{};

		collection.processedFilesCount += 1;

		initiateNextFileTransfer(computerControlInterface, collection);
	}
	else
	{
		vWarning() << "collection" << collectionId << "not found";
	}
}



void FileCollectController::initiateNextFileTransfer(ComputerControlInterface::Pointer computerControlInterface,
													 FileCollection& collection)
{
	collection.state = FileCollection::State::ReadyForNextFileTransfer;

	Q_EMIT collectionChanged(collection.id);

	m_plugin->sendStartFileTransferMessage(collection, computerControlInterface);
}
