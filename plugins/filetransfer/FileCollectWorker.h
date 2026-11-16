// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QRegularExpression>

#include "FileCollection.h"

class FileTransferConfiguration;
class FileTransferPlugin;

// clazy:excludeall=ctor-missing-parent-argument
class FileCollectWorker : public QObject
{
	Q_OBJECT
public:
	enum class TransferState {
		Started,
		WaitingForLockedFile,
		AllFinished
	};

	explicit FileCollectWorker(FileTransferPlugin* plugin,
							   const QString& sourceDirectory = {},
							   const QString& filePattern = {},
							   bool collectRecursively = false);
	~FileCollectWorker() override;

	FileCollection::TransferId currentTransferId() const
	{
		return m_currentTransferId;
	}

	QString currentFileName() const
	{
		return m_files.value(m_currentFileIndex);
	}

	qint64 currentFileSize() const
	{
		return m_currentFile.size();
	}

	const QStringList& files() const
	{
		return m_files;
	}

	QPair<TransferState, QString> startNextTransfer();
	void skipToNextFile();
	void cancelCurrentTransfer();

	QByteArray readNextChunk();
	bool currentFileAtEnd() const;

private:
	void initFiles();
	QList<QRegularExpression> excludeRegExes() const;
	QStringList filterFilesByPattern(const QStringList& files, const QString& pattern) const;

	static constexpr int ChunkSize = 256 * 1024;

	const FileTransferConfiguration& m_configuration;
	QString m_sourceDirectory;
	QString m_filePattern;
	bool m_collectRecursively;

	QStringList m_files;
	int m_currentFileIndex = -1;
	FileCollection::TransferId m_currentTransferId;
	QFile m_currentFile;
};
