// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFile>
#include <QMap>

#include <algorithm>

struct FileCollection
{
	enum class State {
		Invalid,
		Initializing,
		ReadyForNextFileTransfer,
		FileTransferRunning,
		Finished,
		Cancelled
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
	// set by cancel(), applies to all files that were not processed
	QString cancelReason = {};

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
		// a misbehaving computer may announce more files than it listed
		if (processedFilesCount >= 0 && processedFilesCount < files.size())
		{
			failedFiles[processedFilesCount] = reason;
		}
	}

	bool isFailedFile(int index) const
	{
		return failedFiles.contains(index);
	}

	// no further transfer messages are processed for a finished or cancelled collection
	bool isEnded() const
	{
		return state == State::Finished || state == State::Cancelled;
	}

	bool isCancelled() const
	{
		return state == State::Cancelled;
	}

	// failed files plus, for a cancelled collection, all files that were not processed
	bool isNotCollectedFile(int index) const
	{
		return isFailedFile(index) || (isCancelled() && index >= processedFilesCount && index < files.size());
	}

	QString notCollectedReason(int index) const
	{
		return isFailedFile(index) ? failedFiles.value(index) : (isNotCollectedFile(index) ? cancelReason : QString{});
	}

	int notCollectedFilesCount() const
	{
		auto count = int(failedFiles.size());
		if (isCancelled() && processedFilesCount < files.size())
		{
			// the file in transfer at the cancel is already in failedFiles
			count += int(files.size()) - std::max(processedFilesCount, 0);
			if (isFailedFile(processedFilesCount))
			{
				count -= 1;
			}
		}
		return count;
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

	// marks a file still being received as failed, removes it and drops all further transfers;
	// the remaining files count as not collected
	void cancel(const QString& reason)
	{
		if (currentOutputFile)
		{
			markCurrentFileFailed(reason);
			discardCurrentOutputFile();
		}
		cancelReason = reason;
		state = State::Cancelled;
	}

	int currentFileProgress() const
	{
		return currentOutputFile && currentFileSize > 0 ?
			(currentOutputFile->size() * 100 / currentFileSize) :
			0;
	}

	// a cancelled collection keeps the progress of the files processed before the cancel
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
