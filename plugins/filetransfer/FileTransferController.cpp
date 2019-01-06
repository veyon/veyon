/*
 * FileTransferController.cpp - implementation of FileTransferController class
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

#include <QFileInfo>

#include "FileReadThread.h"
#include "FileTransferController.h"
#include "FileTransferPlugin.h"


FileTransferController::FileTransferController( FileTransferPlugin* plugin ) :
	QObject( plugin ),
	m_plugin( plugin ),
	m_currentFileIndex( -1 ),
	m_currentTransferId(),
	m_files(),
	m_flags( Transfer ),
	m_interfaces(),
	m_fileReadThread( nullptr ),
	m_fileState( FileStateFinished ),
	m_processTimer( this )
{
	m_processTimer.setInterval( ProcessInterval );
	connect( &m_processTimer, &QTimer::timeout, this, &FileTransferController::process );
}



FileTransferController::~FileTransferController()
{
	if( m_fileReadThread )
	{
		delete m_fileReadThread;
	}
}



void FileTransferController::setFiles( const QStringList& files )
{
	m_files = files;
	m_currentFileIndex = 0;
	emit filesChanged();
}



void FileTransferController::setInterfaces( const ComputerControlInterfaceList& interfaces )
{
	m_interfaces = interfaces;
}



void FileTransferController::setFlags( Flags flags )
{
	m_flags = flags;
}



void FileTransferController::start()
{
	if( isRunning() == false && m_files.isEmpty() == false )
	{
		m_currentFileIndex = 0;
		m_fileState = FileStateOpen;
		m_processTimer.start();

		emit started();
	}
}



void FileTransferController::stop()
{
	if( isRunning() )
	{
		m_processTimer.stop();

		if( m_fileReadThread )
		{
			delete m_fileReadThread;
			m_fileReadThread = nullptr;
		}

		m_plugin->sendCancelMessage( m_currentTransferId, m_interfaces );
	}

	emit finished();
}



const QStringList& FileTransferController::files() const
{
	return m_files;
}



int FileTransferController::currentFileIndex() const
{
	return m_currentFileIndex;
}



bool FileTransferController::isRunning() const
{
	return m_processTimer.isActive();
}



void FileTransferController::process()
{
	switch( m_fileState )
	{
	case FileStateOpen:
		if( openFile() )
		{
			m_fileState = FileStateTransferring;
		}
		else
		{
			m_fileState = FileStateFinished;
		}
		break;

	case FileStateTransferring:
		if( transferFile() )
		{
			m_fileState = FileStateFinished;
		}
		break;

	case FileStateFinished:
		finishFile();

		if( ++m_currentFileIndex >= m_files.count() )
		{
			if( m_flags.testFlag( OpenTransferFolder ) )
			{
				m_plugin->sendOpenTransferFolderMessage( m_interfaces );
			}

			m_processTimer.stop();
			emit finished();
		}
		else
		{
			m_fileState = FileStateOpen;
		}
		break;
	}

	updateProgress();
}



bool FileTransferController::openFile()
{
	if( m_currentFileIndex >= m_files.count() )
	{
		return false;
	}

	m_fileReadThread = new FileReadThread( m_files[m_currentFileIndex], this );

	if( m_fileReadThread->start() == false )
	{
		delete m_fileReadThread;
		m_fileReadThread = nullptr;
		emit errorOccured( tr( "Could not open file \"%1\" for reading! Please check your permissions!" ).arg( m_currentFileIndex ) );
		return false;
	}

	// start reading initial chunk in background
	m_fileReadThread->readNextChunk( ChunkSize );

	m_currentTransferId = QUuid::createUuid();

	m_plugin->sendStartMessage( m_currentTransferId, QFileInfo( m_files[m_currentFileIndex] ).fileName(),
								m_flags.testFlag( OverwriteExistingFiles ), m_interfaces );

	return true;
}



bool FileTransferController::transferFile()
{
	if( m_fileReadThread == nullptr )
	{
		// something went wrong so finish this file
		return true;
	}

	// wait for all clients to catch up and current data chunk to be read completely
	if( allQueuesEmpty() == false || m_fileReadThread->isChunkReady() == false )
	{
		return false;
	}

	m_plugin->sendDataMessage( m_currentTransferId, m_fileReadThread->currentChunk(), m_interfaces );

	m_fileReadThread->readNextChunk( ChunkSize );

	return m_fileReadThread->atEnd();
}



void FileTransferController::finishFile()
{
	if( m_fileReadThread )
	{
		delete m_fileReadThread;
		m_fileReadThread = nullptr;

		m_plugin->sendFinishMessage( m_currentTransferId, QFileInfo( m_files[m_currentFileIndex] ).fileName(),
									 m_flags.testFlag( OpenFilesInApplication ), m_interfaces );

		m_currentTransferId = QUuid();
	}
}



void FileTransferController::updateProgress()
{
	if( m_files.isEmpty() == false && m_fileReadThread )
	{
		emit progressChanged( m_currentFileIndex * 100 / m_files.count() +
							  m_fileReadThread->progress() / m_files.count() );
	}
	else if( m_files.count() > 0 && m_currentFileIndex >= m_files.count() )
	{
		emit progressChanged( 100 );
	}
}



bool FileTransferController::allQueuesEmpty()
{
	for( const auto& controlInterface : qAsConst(m_interfaces) )
	{
		if( controlInterface->isMessageQueueEmpty() == false )
		{
			return false;
		}
	}

	return true;
}
