/*
 * FileTransferController.h - declaration of FileTransferController class
 *
 * Copyright (c) 2018 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <QTimer>

#include "ComputerControlInterface.h"

class FileReadThread;
class FileTransferPlugin;

class FileTransferController : public QObject
{
	Q_OBJECT
public:
	FileTransferController( FileTransferPlugin* plugin );
	~FileTransferController() override;

	void start( const QStringList& fileList, const ComputerControlInterfaceList& interfaces );
	void stop();

	bool isRunning() const;

signals:
	void errorOccured( const QString& message );
	void progressChanged( const QString& file, int progress );
	void finished();

private:
	void process();
	bool allQueuesEmpty();

	static constexpr int ProcessInterval = 50;
	static constexpr int ChunkSize = 256*1024;

	FileTransferPlugin* m_plugin;

	QString m_currentFile;
	QUuid m_currentTransferId;
	QStringList m_fileList;
	ComputerControlInterfaceList m_interfaces;

	FileReadThread* m_fileReadThread;

	QTimer m_processTimer;

};
