// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QFileInfo>
#include <QRegularExpression>

#include "FileCollectController.h"
#include "FileTransferPlugin.h"
#include "Filesystem.h"
#include "PlatformFilesystemFunctions.h"

namespace {

bool isPathWithinBase(const QString& path, const QString& base)
{
#ifdef Q_OS_WIN
	return path.startsWith(base.endsWith(QLatin1Char('/')) ? base : base + QLatin1Char('/'), Qt::CaseInsensitive);
#else
	return path.startsWith(base.endsWith(QLatin1Char('/')) ? base : base + QLatin1Char('/'));
#endif
}

QString safeGroupingValue(QString value)
{
	static const QRegularExpression invalidFileNameCharacters(QStringLiteral("[\x00-\x1F<>:\"/\\|?*\x7F]"));
	value.replace(invalidFileNameCharacters, QStringLiteral("_"));

	while (value.endsWith(QLatin1Char('.')) || value.endsWith(QLatin1Char(' ')))
	{
		value.chop(1);
	}

	return Filesystem::isSafePathComponent(value) ? value : QStringLiteral("_");
}

}

FileCollectController::FileCollectController(FileTransferPlugin* plugin) :
	QObject(plugin),
	m_plugin(plugin),
	m_destinationDirectory(VeyonCore::filesystem().expandPath(plugin->configuration().collectedFilesDestinationDirectory())),
	m_collectionDirectory(configuration().collectionDirectory()),
	m_collectedFilesGroupingMode(configuration().collectedFilesGroupingMode()),
	m_collectedFilesGroupingAttributes({configuration().collectedFilesGroupingAttribute1(),
										configuration().collectedFilesGroupingAttribute2(),
										configuration().collectedFilesGroupingAttribute3()}),
	m_collectSourceDirectory(),
	m_filePattern()
{
	connect(this, &FileCollectController::collectionChanged, this, &FileCollectController::overallProgressChanged);
}



FileCollectController::~FileCollectController()
{
}



const FileTransferConfiguration& FileCollectController::configuration() const
{
	return m_plugin->configuration();
}



void FileCollectController::setCollectSourceDirectory(const QString& sourceDir)
{
	m_collectSourceDirectory = sourceDir;
}



void FileCollectController::setFilePattern(const QString& pattern)
{
	m_filePattern = pattern;
}



void FileCollectController::setSubfolderHandling(SubfolderHandling subfolderHandling)
{
	m_subfolderHandling = subfolderHandling;
}



void FileCollectController::setDestinationDirectory(const QString& destDir)
{
	m_destinationDirectory = destDir;
}



void FileCollectController::setInterfaces(const ComputerControlInterfaceList& computerControlInterfaces)
{
	if (isRunning() == false)
	{
		Q_EMIT collectionsAboutToChange();

		m_collections.clear();

		for (const auto& computerControlInterface : computerControlInterfaces)
		{
			if (computerControlInterface->state() == ComputerControlInterface::State::Connected &&
				computerControlInterface->userLoginName().isEmpty() == false)
			{
				FileCollection collection{};
				collection.state = FileCollection::State::Initializing;
				collection.name = computerControlInterface->userFullName().isEmpty() ?
					computerControlInterface->userLoginName() :
					computerControlInterface->userFullName();
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
	const auto baseDirectory = outputDirectory();
	if (m_collectedFilesGroupingMode == CollectedFilesGroupingMode::None)
	{
		return Filesystem::safeJoin(baseDirectory, fileName);
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

	const auto rawGroupingValue = groupingValues.join(QLatin1Char('_'));
	if (rawGroupingValue.isEmpty())
	{
		return Filesystem::safeJoin(baseDirectory, fileName);
	}
	const auto groupingValue = safeGroupingValue(rawGroupingValue);

	switch (m_collectedFilesGroupingMode)
	{
	case CollectedFilesGroupingMode::CreateSubdirectories:
		return Filesystem::safeJoin(baseDirectory, groupingValue + QLatin1Char('/') + fileName);
	case CollectedFilesGroupingMode::PrefixFilenames:
		return Filesystem::safeJoin(baseDirectory, groupingValue + QStringLiteral("_") + fileName);
	case CollectedFilesGroupingMode::None:
		break;
	}

	return {};
}



void FileCollectController::start()
{
	if (isRunning() == false && m_collections.isEmpty() == false)
	{
		for (auto it = m_collections.constBegin(), end = m_collections.constEnd(); it != end; ++it)
		{
			m_plugin->sendInitFileCollectionMessage(it.value(), it.key(),
													m_collectSourceDirectory, m_filePattern, m_subfolderHandling);
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
		0 :
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

		if (fileSize < 0)
		{
			vCritical() << "rejecting file transfer with invalid file size";
			collection.processedFilesCount += 1;
			initiateNextFileTransfer(computerControlInterface, collection);
			return;
		}

		const auto outputFilePath = this->outputFilePath(computerControlInterface, fileName);
		if (outputFilePath.isEmpty())
		{
			vCritical() << "rejecting file transfer with unsafe output path";
			collection.processedFilesCount += 1;
			initiateNextFileTransfer(computerControlInterface, collection);
			return;
		}

		collection.currentOutputFile = new QFile(outputFilePath);
		const auto effectiveOutputPath = QFileInfo(collection.currentOutputFile->fileName()).absolutePath();
		if (VeyonCore::filesystem().ensurePathExists(effectiveOutputPath) == false)
		{
			vCritical() << "failed to create directory" << effectiveOutputPath;
			delete collection.currentOutputFile;
			collection.currentOutputFile = nullptr;
			return;
		}

		const auto canonicalBase = QDir(outputDirectory()).canonicalPath();
		const auto canonicalOutputDirectory = QDir(effectiveOutputPath).canonicalPath();
		if (canonicalBase.isEmpty() || canonicalOutputDirectory.isEmpty() ||
			(canonicalOutputDirectory != canonicalBase &&
			 isPathWithinBase(canonicalOutputDirectory, canonicalBase) == false))
		{
			vCritical() << "output path resolved outside collection directory";
			delete collection.currentOutputFile;
			collection.currentOutputFile = nullptr;
			collection.processedFilesCount += 1;
			initiateNextFileTransfer(computerControlInterface, collection);
			return;
		}

		// openFileSafely() rejects a symlink target. Replacing a checked parent directory
		// afterwards still requires local write access to the collection tree.
		if (VeyonCore::platform().filesystemFunctions().openFileSafely(
				collection.currentOutputFile,
				QFile::WriteOnly | QFile::Truncate,
				QFile::ReadOwner | QFile::WriteOwner) == false)
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
			if (dataChunk.size() > collection.currentFileSize - collection.currentOutputFile->size())
			{
				vCritical() << "file transfer exceeds announced file size";
				const auto outputFilePath = collection.currentOutputFile->fileName();
				collection.currentOutputFile->close();
				delete collection.currentOutputFile;
				collection.currentOutputFile = nullptr;
				QFile::remove(outputFilePath);
				collection.currentFileSize = 0;
				collection.currentTransferId = FileCollection::TransferId{};
				collection.processedFilesCount += 1;
				initiateNextFileTransfer(computerControlInterface, collection);
				return;
			}

			if (collection.currentOutputFile->write(dataChunk) != dataChunk.size())
			{
				vCritical() << "failed to write file transfer data";
				const auto outputFilePath = collection.currentOutputFile->fileName();
				collection.currentOutputFile->close();
				delete collection.currentOutputFile;
				collection.currentOutputFile = nullptr;
				QFile::remove(outputFilePath);
				collection.currentFileSize = 0;
				collection.currentTransferId = FileCollection::TransferId{};
				collection.processedFilesCount += 1;
				initiateNextFileTransfer(computerControlInterface, collection);
				return;
			}
		}
		else
		{
			vCritical() << "transfer not started / output file not available" << transferId;
			return;
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
