// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFile>
#include <QMutex>
#include <QThread>
#include <QTimer>

class FileReadThread : public QObject
{
	Q_OBJECT
public:
	explicit FileReadThread(const QString& fileName, QObject* parent = nullptr);
	~FileReadThread() override;

	bool start();

	QByteArray currentChunk();
	void readNextChunk(qint64 chunkSize);
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
