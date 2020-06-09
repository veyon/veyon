/*
 * FileTransferPlugin.cpp - implementation of FileTransferPlugin class
 *
 * Copyright (c) 2018-2020 Tobias Junghans <tobydox@veyon.io>
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

#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QQuickWindow>

#include "BuiltinFeatures.h"
#include "FileTransferController.h"
#include "FileTransferDialog.h"
#include "FileTransferPlugin.h"
#include "FileTransferUserConfiguration.h"
#include "FeatureWorkerManager.h"
#include "PlatformFilesystemFunctions.h"
#include "QmlCore.h"
#include "SystemTrayIcon.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"


FileTransferPlugin::FileTransferPlugin( QObject* parent ) :
	QObject( parent ),
	m_fileTransferFeature( QStringLiteral( "FileTransfer" ),
						   Feature::Action | Feature::AllComponents,
						   Feature::Uid( "4a70bd5a-fab2-4a4b-a92a-a1e81d2b75ed" ),
						   Feature::Uid(),
						   tr( "File transfer" ), {},
						   tr( "Click this button to transfer files from your computer to all computers." ),
						   QStringLiteral(":/filetransfer/applications-office.png") ),
	m_features( { m_fileTransferFeature } )
{
}



FileTransferPlugin::~FileTransferPlugin()
{
	delete m_fileTransferController;
}



bool FileTransferPlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
									   const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature != m_fileTransferFeature )
	{
		return false;
	}

	const auto userConfigObject = master.userConfigurationObject();

	m_lastFileTransferSourceDirectory = FileTransferUserConfiguration( userConfigObject ).lastFileTransferSourceDirectory();

	if( master.appWindow() )
	{
		auto dialog = VeyonCore::qmlCore().createObjectFromFile( QStringLiteral("qrc:/filetransfer/FileTransferFileDialog.qml"),
														 master.appWindow(),
														 this );
		connect( this, &FileTransferPlugin::acceptSelectedFiles, dialog, // clazy:exclude=connect-non-signal
				 [this, computerControlInterfaces, userConfigObject]( const QList<QUrl>& fileUrls )
		{
			QStringList files;
			files.reserve( fileUrls.size() );
			for( const auto& url : fileUrls )
			{
				files.append( url.toString( QUrl::RemoveScheme ) );
			}
			startFileTransfer( files, userConfigObject, computerControlInterfaces );
		} );

		return true;
	}

	auto files = QFileDialog::getOpenFileNames( master.mainWindow(),
												tr( "Select one or more files to transfer" ),
												lastFileTransferSourceDirectory() );

	if( files.isEmpty() == false )
	{
		startFileTransfer( files, userConfigObject, computerControlInterfaces );

		auto dialog = new FileTransferDialog( m_fileTransferController, master.mainWindow() );
		connect( dialog, &QDialog::finished, dialog, &QDialog::deleteLater );
		dialog->exec();
	}

	return true;
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

		if( message.command() == FileTransferFinishCommand )
		{
			VeyonCore::builtinFeatures().systemTrayIcon().showMessage( m_fileTransferFeature.displayName(),
																	   tr( "Received file \"%1\"." ).
																	   arg( message.argument( Filename ).toString() ),
																	   server.featureWorkerManager() );
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
			m_currentFile.setFileName( QDir::homePath() + QDir::separator() + message.argument( Filename ).toString() );
			if( m_currentFile.exists() && message.argument( OverwriteExistingFile ).toBool() == false )
			{
				QMessageBox::critical( nullptr, m_fileTransferFeature.displayName(),
									   tr( "Could not receive file \"%1\" as it already exists." ).
									   arg( m_currentFile.fileName() ) );
				return true;
			}

			if( VeyonCore::platform().filesystemFunctions().openFileSafely(
					&m_currentFile,
					QFile::WriteOnly | QFile::Truncate,
					QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther ) )
			{
				m_currentTransferId = message.argument( TransferId ).toUuid();
			}
			else
			{
				QMessageBox::critical( nullptr, m_fileTransferFeature.displayName(),
									   tr( "Could not receive file \"%1\" as it could not be opened for writing!" ).
									   arg( m_currentFile.fileName() ) );
			}
			return true;

		case FileTransferContinueCommand:
			if( message.argument( TransferId ).toUuid() == m_currentTransferId )
			{
				m_currentFile.write( message.argument( DataChunk ).toByteArray() );
			}
			else
			{
				vWarning() << "received chunk for unknown transfer ID";
			}
			return true;

		case FileTransferCancelCommand:
			if( message.argument( TransferId ).toUuid() == m_currentTransferId )
			{
				m_currentFile.remove();
			}
			else
			{
				vWarning() << "received chunk for unknown transfer ID";
			}
			return true;

		case FileTransferFinishCommand:
			m_currentFile.close();
			if( message.argument( OpenFileInApplication ).toBool() )
			{
				QDesktopServices::openUrl( QUrl::fromLocalFile( m_currentFile.fileName() ) );
			}
			m_currentFile.setFileName( {} );
			return true;

		case OpenTransferFolder:
			QDesktopServices::openUrl( QUrl::fromLocalFile( QDir::homePath() ) );
			return true;

		default:
			break;
		}
	}

	return false;
}



void FileTransferPlugin::sendStartMessage( QUuid transferId, const QString& fileName,
										   bool overwriteExistingFile, const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage( FeatureMessage( m_fileTransferFeature.uid(), FileTransferStartCommand ).
						addArgument( TransferId, transferId ).
						addArgument( Filename, fileName ).
						addArgument( OverwriteExistingFile, overwriteExistingFile ),
						interfaces );
}



void FileTransferPlugin::sendDataMessage( QUuid transferId, const QByteArray& data,
										  const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage( FeatureMessage( m_fileTransferFeature.uid(), FileTransferContinueCommand ).
						addArgument( TransferId, transferId ).
						addArgument( DataChunk, data ),
						interfaces );
}



void FileTransferPlugin::sendCancelMessage( QUuid transferId,
											const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage( FeatureMessage( m_fileTransferFeature.uid(), FileTransferCancelCommand ).
						addArgument( TransferId, transferId ), interfaces );
}



void FileTransferPlugin::sendFinishMessage( QUuid transferId, const QString& fileName,
											bool openFileInApplication, const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage( FeatureMessage( m_fileTransferFeature.uid(), FileTransferFinishCommand ).
						addArgument( TransferId, transferId ).
						addArgument( Filename, fileName ).
						addArgument( OpenFileInApplication, openFileInApplication ), interfaces );
}



void FileTransferPlugin::sendOpenTransferFolderMessage( const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage( FeatureMessage( m_fileTransferFeature.uid(), OpenTransferFolder ), interfaces );
}



void FileTransferPlugin::startFileTransfer( const QStringList& files, Configuration::Object* userConfigObject,
											const ComputerControlInterfaceList& interfaces )
{
	FileTransferUserConfiguration( userConfigObject ).
			setLastFileTransferSourceDirectory( QFileInfo( files.first() ).absolutePath() );

	if( m_fileTransferController == nullptr )
	{
		m_fileTransferController = new FileTransferController( this );
	}

	auto relativeFiles = files;

	for( auto& file : relativeFiles )
	{
		QFileInfo fileInfo( file );
		if( fileInfo.dir() == QDir::current() )
		{
			file = fileInfo.fileName();
		}
	}

	m_fileTransferController->setFiles( relativeFiles );
	m_fileTransferController->setInterfaces( interfaces );
}
