/*
 * FileCollectWorker.cpp - implementation of FileCollectWorker class
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

#include <QDir>
#include <QFileInfo>

#include "FileCollectWorker.h"
#include "FileTransferPlugin.h"
#include "Filesystem.h"


static QStringList listFilesInDirectory(const QString& dirPath, bool recursive = false)
{
	QStringList fileList;
	QDir dir(dirPath);

	if (!dir.exists())
		return fileList;

	const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
	fileList.reserve(entries.size());

	for (const QFileInfo& fi : entries)
	{
		fileList.append(fi.absoluteFilePath());
	}

	if (recursive)
	{
		const QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
		for (const QFileInfo& subdirInfo : subdirs)
		{
			fileList.append(listFilesInDirectory(subdirInfo.absoluteFilePath(), true));
		}
	}

	return fileList;
}



FileCollectWorker::FileCollectWorker(FileTransferPlugin* plugin) :
	QObject(plugin),
	m_sourceDirectory(VeyonCore::filesystem().expandPath(plugin->configuration().filesToCollectSourceDirectory())),
	m_collectFilesRecursively(plugin->configuration().collectFilesRecursively())
{
	if (m_sourceDirectory.endsWith(QDir::separator()) == false)
	{
		m_sourceDirectory.append(QDir::separator());
	}

	m_files = listFilesInDirectory(m_sourceDirectory, m_collectFilesRecursively);

	for (auto it = m_files.begin(); it != m_files.end(); ) // clazy:exclude=detaching-member
	{
		if (it->startsWith(m_sourceDirectory))
		{
			*it = it->mid(m_sourceDirectory.size());
			++it;
		}
		else
		{
			it = m_files.erase(it);
		}
	}
}



FileCollectWorker::~FileCollectWorker()
{
}



bool FileCollectWorker::startNextTransfer()
{
	if (m_currentFileIndex + 1 >= m_files.count())
	{
		m_currentTransferId = FileCollection::TransferId{};
		return false;
	}

	++m_currentFileIndex;
	m_currentTransferId = QUuid::createUuid();
	m_currentFile.close();
	m_currentFile.setFileName(m_sourceDirectory + std::as_const(m_files)[m_currentFileIndex]);
	if (m_currentFile.open(QFile::ReadOnly))
	{
		vCritical() << "file opened" << m_currentFile.fileName();
		return true;
	}

	vCritical() << "file not opened";
	return startNextTransfer();
}



void FileCollectWorker::cancelCurrentTransfer()
{
	m_currentFile.close();
	m_currentTransferId = FileCollection::TransferId{};
}



QByteArray FileCollectWorker::readNextChunk()
{
	return m_currentFile.read(ChunkSize);
}



bool FileCollectWorker::currentFileAtEnd() const
{
	return m_currentFile.atEnd();
}
