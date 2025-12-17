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


static QStringList listFilesInDirectory(const QString& dirPath, const QList<QRegularExpression>& excludeRegExes, bool recursive = false)
{
	QStringList fileList;
	QDir dir(dirPath);

	if (!dir.exists())
		return fileList;

	const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
	fileList.reserve(entries.size());

	for (const QFileInfo& fi : entries)
	{
		bool exclude = false;
		for (const auto& excludeRegEx : excludeRegExes)
		{
			if (excludeRegEx.match(fi.fileName()).hasMatch())
			{
				vDebug() << "excluding" << fi.fileName() << "as it matches" << excludeRegEx;
				exclude = true;
				break;
			}
		}

		if (exclude == false)
		{
			fileList.append(QDir::toNativeSeparators(fi.absoluteFilePath()));
		}
	}

	if (recursive)
	{
		const QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
		for (const QFileInfo& subdirInfo : subdirs)
		{
			fileList.append(listFilesInDirectory(subdirInfo.absoluteFilePath(), excludeRegExes, true));
		}
	}

	return fileList;
}



FileCollectWorker::FileCollectWorker(FileTransferPlugin* plugin) :
	QObject(plugin),
	m_configuration(plugin->configuration()),
	m_sourceDirectory(VeyonCore::filesystem().expandPath(m_configuration.filesToCollectSourceDirectory()))
{
	if (m_sourceDirectory.endsWith(QDir::separator()) == false)
	{
		m_sourceDirectory.append(QDir::separator());
	}

	initFiles();
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



void FileCollectWorker::initFiles()
{
	m_files = listFilesInDirectory(m_sourceDirectory, excludeRegExes(), m_configuration.collectFilesRecursively());

	for (auto it = m_files.begin(); it != m_files.end(); ) // clazy:exclude=detaching-member
	{
#ifdef Q_OS_WIN
		const auto caseSensitivity = Qt::CaseInsensitive;
#else
		const auto caseSensitivity = Qt::CaseSensitive;
#endif
		if (it->startsWith(m_sourceDirectory, caseSensitivity))
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



QList<QRegularExpression> FileCollectWorker::excludeRegExes() const
{
	auto excludePatterns = m_configuration.filesToExcludeFromCollecting().replace(u';', u' ')
								 .replace(u',', u' ').split(u' ');
	excludePatterns.removeAll(QString{});

	QList<QRegularExpression> regExes;
	std::transform(excludePatterns.begin(), excludePatterns.end(),
				   std::back_inserter(regExes),
				   [](const QString& pattern) {
			return QRegularExpression(QString(pattern).replace(u'*', QStringLiteral(".*")),
									  QRegularExpression::CaseInsensitiveOption);
	});

	return regExes;
}
