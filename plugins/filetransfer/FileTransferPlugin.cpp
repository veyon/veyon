/*
 * FileTransferPlugin.cpp - implementation of FileTransferPlugin class
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

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "FileTransferPlugin.h"
#include "FeatureWorkerManager.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


FileTransferPlugin::FileTransferPlugin( QObject* parent ) :
	QObject( parent ),
	m_fileTransferFeature( Feature::Action | Feature::AllComponents,
						   Feature::Uid( "4a70bd5a-fab2-4a4b-a92a-a1e81d2b75ed" ),
						   Feature::Uid(),
						   tr( "File transfer" ), QString(),
						   tr( "Click this button to transfer files from your computer to all "
							   "computers." ),
						   QStringLiteral(":/filetransfer/applications-office.png") ),
	m_features( { m_fileTransferFeature } ),
	m_fileList(),
	m_currentFile(),
	m_activeTransferInterfaces(),
	m_processTimer( this )
{
	connect( &m_processTimer, &QTimer::timeout, this, &FileTransferPlugin::processFileTransfer );
}



bool FileTransferPlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
									   const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(master)

	if( feature == m_fileTransferFeature )
	{
		const auto files = QFileDialog::getOpenFileNames( master.mainWindow(),
														  tr( "Select one or more files to transfer" ),
														  QDir::homePath() );
		if( files.isEmpty() == false )
		{
			m_fileList = files;
			m_activeTransferInterfaces = computerControlInterfaces;
			m_processTimer.start();
		}

		return true;
	}

	return false;
}



bool FileTransferPlugin::stopFeature( VeyonMasterInterface& master, const Feature& feature,
									  const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(master)
	Q_UNUSED(feature)
	Q_UNUSED(computerControlInterfaces)

	return false;
}



bool FileTransferPlugin::handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
											   ComputerControlInterface::Pointer computerControlInterface )
{
	Q_UNUSED(master)
	Q_UNUSED(message)
	Q_UNUSED(computerControlInterface)

	return false;
}



bool FileTransferPlugin::handleFeatureMessage( VeyonServerInterface& server,
											   const MessageContext& messageContext,
											   const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( m_fileTransferFeature.uid() == message.featureUid() )
	{
		if( server.featureWorkerManager().isWorkerRunning( m_fileTransferFeature ) == false )
		{
			server.featureWorkerManager().startWorker( m_fileTransferFeature, FeatureWorkerManager::UnmanagedSessionProcess );
		}

		// forward message to worker
		server.featureWorkerManager().sendMessage( message );

		return true;
	}

	return false;
}



bool FileTransferPlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker)

	if( m_fileTransferFeature.uid() == message.featureUid() )
	{
		switch( message.command() )
		{
		case FileTransferStartCommand:
			m_currentFile.close();
			// TODO: make path configurable
			m_currentFile.setFileName( QDir::homePath() + QDir::separator() + message.argument( FilenameArgument ).toString() );
			// TODO: check for existing files
			if( m_currentFile.open( QFile::WriteOnly | QFile::Truncate ) == false )
			{
				QMessageBox::critical( nullptr, tr( "File transfer"),
									   tr( "Could not open file \"%1\" for writing!" ).arg( m_currentFile.fileName() ) );
			}
			return true;

		case FileTransferContinueCommand:
			m_currentFile.write( message.argument( FileDataChunk ).toByteArray() );
			return true;

		case FileTransferFinishCommand:
			m_currentFile.close();
			return true;

		default:
			break;
		}
	}

	return false;
}



void FileTransferPlugin::processFileTransfer()
{
	// wait for all clients to catch up
	if( allQueuesEmpty() == false )
	{
		return;
	}

	if( m_currentFile.isOpen() )
	{
		sendFeatureMessage( FeatureMessage( m_fileTransferFeature.uid(), FileTransferContinueCommand ).
							addArgument( FileDataChunk, m_currentFile.read( ChunkSize ) ),
							m_activeTransferInterfaces );

		if( m_currentFile.atEnd() )
		{
			m_currentFile.close();
			sendFeatureMessage( FeatureMessage( m_fileTransferFeature.uid(), FileTransferFinishCommand ),
								m_activeTransferInterfaces );
		}
		return;
	}

	if( m_fileList.isEmpty() )
	{
		m_processTimer.stop();
		return;
	}

	m_currentFile.setFileName( m_fileList.takeFirst() );
	if( m_currentFile.open( QFile::ReadOnly ) == false )
	{
		QMessageBox::critical( nullptr, tr( "File transfer" ),
							   tr( "Could not open file \"%1\" for reading! Please check your permissions!" ).arg( m_currentFile.fileName() ) );
		return;
	}

	sendFeatureMessage( FeatureMessage( m_fileTransferFeature.uid(), FileTransferStartCommand ).
						addArgument( FilenameArgument, QFileInfo( m_currentFile.fileName() ).fileName() ),
						m_activeTransferInterfaces );
}



bool FileTransferPlugin::allQueuesEmpty()
{
	for( auto controlInterface : m_activeTransferInterfaces )
	{
		if( controlInterface->isMessageQueueEmpty() == false )
		{
			return false;
		}
	}

	return true;
}
