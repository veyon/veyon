/*
 * FileTransferController.cpp - implementation of FileTransferController class
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

#include <QFileInfo>

#include "FileReadThread.h"
#include "FileTransferController.h"
#include "FileTransferPlugin.h"


FileTransferController::FileTransferController( FileTransferPlugin* plugin ) :
	QObject( plugin ),
	m_plugin( plugin ),
	m_currentFile(),
	m_currentTransferId(),
	m_fileList(),
	m_interfaces(),
	m_fileReadThread( nullptr ),
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



void FileTransferController::start( const QStringList& fileList, const ComputerControlInterfaceList& interfaces )
{
	if( isRunning() == false )
	{
		m_fileList = fileList;
		m_interfaces = interfaces;
		m_processTimer.start();
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



bool FileTransferController::isRunning() const
{
	return m_processTimer.isActive();
}



void FileTransferController::process()
{
	// wait for all clients to catch up and current data chunk to be read completely
	if( allQueuesEmpty() == false ||
			( m_fileReadThread && m_fileReadThread->isChunkReady() == false ) )
	{
		return;
	}

	if( m_fileReadThread )
	{
		m_plugin->sendDataMessage( m_currentTransferId, m_fileReadThread->currentChunk(), m_interfaces );

		emit progressChanged( m_currentFile, m_fileReadThread->progress() );

		m_fileReadThread->readNextChunk( ChunkSize );

		if( m_fileReadThread->atEnd() )
		{
			delete m_fileReadThread;
			m_plugin->sendFinishMessage( m_currentTransferId, m_interfaces );
		}
		return;
	}

	if( m_fileList.isEmpty() )
	{
		m_processTimer.stop();
		emit finished();
		return;
	}

	m_currentFile = m_fileList.takeFirst();
	m_currentTransferId = QUuid::createUuid();

	m_fileReadThread = new FileReadThread( m_currentFile, this );

	if( m_fileReadThread->start() == false )
	{
		emit errorOccured( tr( "Could not open file \"%1\" for reading! Please check your permissions!" ).arg( m_currentFile ) );
		return;
	}

	// start reading initial chunk in background
	m_fileReadThread->readNextChunk( ChunkSize );

	m_plugin->sendStartMessage( m_currentTransferId, QFileInfo( m_currentFile ).fileName(), m_interfaces );
}



bool FileTransferController::allQueuesEmpty()
{
	for( auto controlInterface : m_interfaces )
	{
		if( controlInterface->isMessageQueueEmpty() == false )
		{
			return false;
		}
	}

	return true;
}
