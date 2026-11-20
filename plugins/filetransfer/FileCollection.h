// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFile>
#include <QMap>

struct FileCollection
{
	enum class State {
		Invalid,
		Initializing,
		ReadyForNextFileTransfer,
		FileTransferRunning,
		Finished
	};

	using Id = QUuid;
	using TransferId = QUuid;

	Id id = QUuid::createUuid();

	State state = State::Invalid;

	QString name = {};
	QStringList files = {};
	int processedFilesCount = 0;
	// files that were skipped or rejected (index in files → reason)
	QMap<int, QString> failedFiles = {};

	TransferId currentTransferId = {};
	QFile* currentOutputFile = nullptr;
	// QFile::fileName() is empty after opening a file via openFileSafely()
	QString currentOutputFilePath = {};
	qint64 currentFileSize = 0;

	bool operator!=(const FileCollection& other) const
	{
		return other.id != id;
	}

	void markCurrentFileFailed(const QString& reason)
	{
		failedFiles[processedFilesCount] = reason;
	}

	bool isFailedFile(int index) const
	{
		return failedFiles.contains(index);
	}

	void closeCurrentOutputFile()
	{
		if (currentOutputFile)
		{
			currentOutputFile->close();
			delete currentOutputFile;
			currentOutputFile = nullptr;
		}
		currentOutputFilePath.clear();
		currentFileSize = 0;
		currentTransferId = TransferId{};
	}

	// closes and removes a partially received or rejected output file
	void discardCurrentOutputFile()
	{
		const auto outputFilePath = currentOutputFilePath;
		closeCurrentOutputFile();
		if (outputFilePath.isEmpty() == false)
		{
			QFile::remove(outputFilePath);
		}
	}

	int currentFileProgress() const
	{
		return currentOutputFile && currentFileSize > 0 ?
			(currentOutputFile->size() * 100 / currentFileSize) :
			0;
	}

	int progress() const
	{
		if (files.isEmpty())
		{
			return state == State::Finished ? 100 : 0;
		}
		if (processedFilesCount >= files.size())
		{
			return 100;
		}
		return processedFilesCount * 100 / files.size() +
			currentFileProgress() / files.size();
	}
};
