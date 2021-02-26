/*
 * FileTransferController.h - declaration of FileTransferController class
 *
 * Copyright (c) 2018-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QTimer>

#include "ComputerControlInterface.h"

class FileReadThread;
class FileTransferPlugin;

class FileTransferController : public QObject
{
	Q_OBJECT
public:
	enum Flag {
		Transfer = 0x00,
		OpenFilesInApplication = 0x01,
		OpenTransferFolder = 0x02,
		OverwriteExistingFiles = 0x04,
	};
	Q_DECLARE_FLAGS(Flags, Flag)
	Q_FLAG(Flags)

	explicit FileTransferController( FileTransferPlugin* plugin );
	~FileTransferController() override;

	void setFiles( const QStringList& files );
	void setInterfaces( const ComputerControlInterfaceList& interfaces );
	void setFlags( Flags flags );

	void start();
	void stop();

	const QStringList& files() const;

	int currentFileIndex() const;

	bool isRunning() const;

Q_SIGNALS:
	void errorOccured( const QString& message );
	void filesChanged();
	void progressChanged( int progress );
	void started();
	void finished();

private:
	enum FileState {
		FileStateOpen,
		FileStateTransferring,
		FileStateFinished
	};

	void process();

	bool openFile();
	bool transferFile();
	void finishFile();

	void updateProgress();

	bool allQueuesEmpty();

	static constexpr int ProcessInterval = 25;
	static constexpr int ChunkSize = 256*1024;

	FileTransferPlugin* m_plugin;

	int m_currentFileIndex;
	QUuid m_currentTransferId;
	QStringList m_files;
	Flags m_flags;
	ComputerControlInterfaceList m_interfaces;

	FileReadThread* m_fileReadThread;

	FileState m_fileState;

	QTimer m_processTimer;

};
