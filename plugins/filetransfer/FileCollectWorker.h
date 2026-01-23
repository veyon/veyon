/*
 * FileCollectWorker.h - declaration of FileCollectWorker class
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

	explicit FileCollectWorker(FileTransferPlugin* plugin);
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

	static constexpr int ChunkSize = 256*1024;

	const FileTransferConfiguration& m_configuration;
	QString m_sourceDirectory;

	QStringList m_files;
	int m_currentFileIndex = -1;
	FileCollection::TransferId m_currentTransferId;
	QFile m_currentFile;

};
