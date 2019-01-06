/*
 * FileReadThread.h - declaration of FileReadThread class
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QFile>
#include <QMutex>
#include <QTimer>
#include <QThread>

class FileReadThread : public QObject
{
	Q_OBJECT
public:
	FileReadThread( const QString& fileName, QObject* parent = nullptr );
	~FileReadThread() override;

	bool start();

	QByteArray currentChunk();
	void readNextChunk( qint64 chunkSize );
	bool isChunkReady();

	bool atEnd();
	int progress();

private:
	QMutex m_mutex;
	QThread* m_thread;
	QFile* m_file;
	QByteArray m_currentChunk;

	QTimer* m_timer;

	QString m_fileName;
	bool m_chunkReady;
	qint64 m_filePos;
	qint64 m_fileSize;

};
