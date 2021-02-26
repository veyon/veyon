/*
 * FileTransferPlugin.cpp - implementation of FileTransferPlugin class
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

#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "BuiltinFeatures.h"
#include "Filesystem.h"
#include "FileTransferConfigurationPage.h"
#include "FileTransferController.h"
#include "FileTransferDialog.h"
#include "FileTransferPlugin.h"
#include "FileTransferUserConfiguration.h"
#include "FeatureWorkerManager.h"
#include "PlatformFilesystemFunctions.h"
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
	m_features( { m_fileTransferFeature } ),
	m_configuration( &VeyonCore::config() )
{
}



FileTransferPlugin::~FileTransferPlugin()
{
	delete m_fileTransferController;
}



bool FileTransferPlugin::controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
										const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( hasFeature( featureUid ) == false )
	{
		return false;
	}

	if( operation == Operation::Initialize )
	{
		auto files = arguments.value( argToString(Argument::Files) ).toStringList();
		if( files.isEmpty() )
		{
			return false;
		}

		for( auto& file : files )
		{
			QFileInfo fileInfo( file );
			if( fileInfo.dir() == QDir::current() )
			{
				file = fileInfo.fileName();
			}
		}

		if( m_fileTransferController == nullptr )
		{
			m_fileTransferController = new FileTransferController( this );
		}

		m_fileTransferController->setFiles( files );
		m_fileTransferController->setInterfaces( computerControlInterfaces );

		return true;
	}

	if( operation == Operation::Start )
	{
		if( m_fileTransferController )
		{
			m_fileTransferController->start();
		}

		return true;
	}

	if( operation == Operation::Stop )
	{
		if( m_fileTransferController )
		{
			m_fileTransferController->stop();
		}

		return true;
	}

	return false;
}



bool FileTransferPlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
									   const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature == m_fileTransferFeature )
	{
		FileTransferUserConfiguration config( master.userConfigurationObject() );

		m_lastFileTransferSourceDirectory = config.lastFileTransferSourceDirectory();
		if( m_configuration.rememberLastFileTransferSourceDirectory() == false ||
			QDir( m_lastFileTransferSourceDirectory ).exists() == false )
		{
			m_lastFileTransferSourceDirectory = VeyonCore::filesystem().expandPath( m_configuration.fileTransferDefaultSourceDirectory() );
		}
		const auto files = QFileDialog::getOpenFileNames( master.mainWindow(),
														  tr( "Select one or more files to transfer" ),
														  m_lastFileTransferSourceDirectory );

		if( files.isEmpty() == false )
		{
			config.setLastFileTransferSourceDirectory( QFileInfo( files.first() ).absolutePath() );

			if( controlFeature( feature.uid(), Operation::Initialize,
								{ { argToString(Argument::Files), files } }, computerControlInterfaces ) )
			{
				auto dialog = new FileTransferDialog( m_fileTransferController, master.mainWindow() );
				connect( dialog, &QDialog::finished, dialog, &QDialog::deleteLater );
				dialog->exec();
			}
		}

		return true;
	}

	return false;
}



bool FileTransferPlugin::handleFeatureMessage( VeyonServerInterface& server,
											   const MessageContext& messageContext,
											   const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( m_fileTransferFeature.uid() == message.featureUid() )
	{
		if( message.command() == FileTransferFinishCommand )
		{
			VeyonCore::builtinFeatures().systemTrayIcon().showMessage( m_fileTransferFeature.displayName(),
																   tr( "Received file \"%1\"." ).
																	   arg( message.argument( Argument::Filename ).toString() ),
																   server.featureWorkerManager() );
		}

		// forward message to worker
		server.featureWorkerManager().sendMessageToUnmanagedSessionWorker( message );

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

			m_currentFileName = destinationDirectory() + QDir::separator() + message.argument( Argument::Filename ).toString();
			m_currentFile.setFileName( m_currentFileName );
			if( m_currentFile.exists() && message.argument( Argument::OverwriteExistingFile ).toBool() == false )
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
				m_currentTransferId = message.argument( Argument::TransferId ).toUuid();
			}
			else
			{
				QMessageBox::critical( nullptr, m_fileTransferFeature.displayName(),
									   tr( "Could not receive file \"%1\" as it could not be opened for writing!" ).
									   arg( m_currentFile.fileName() ) );
			}
			return true;

		case FileTransferContinueCommand:
			if( message.argument( Argument::TransferId ).toUuid() == m_currentTransferId )
			{
				m_currentFile.write( message.argument( Argument::DataChunk ).toByteArray() );
			}
			else
			{
				vWarning() << "received chunk for unknown transfer ID";
			}
			return true;

		case FileTransferCancelCommand:
			if( message.argument( Argument::TransferId ).toUuid() == m_currentTransferId )
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
			if( message.argument( Argument::OpenFileInApplication ).toBool() )
			{
				QDesktopServices::openUrl( QUrl::fromLocalFile( m_currentFileName ) );
			}
			m_currentFile.setFileName( {} );
			return true;

		case OpenTransferFolder:
			QDesktopServices::openUrl( QUrl::fromLocalFile( destinationDirectory() ) );
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
						addArgument( Argument::TransferId, transferId ).
						addArgument( Argument::Filename, fileName ).
						addArgument( Argument::OverwriteExistingFile, overwriteExistingFile ),
						interfaces );
}



void FileTransferPlugin::sendDataMessage( QUuid transferId, const QByteArray& data,
										  const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage( FeatureMessage( m_fileTransferFeature.uid(), FileTransferContinueCommand ).
						addArgument( Argument::TransferId, transferId ).
						addArgument( Argument::DataChunk, data ),
						interfaces );
}



void FileTransferPlugin::sendCancelMessage( QUuid transferId,
											const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage( FeatureMessage( m_fileTransferFeature.uid(), FileTransferCancelCommand ).
						addArgument( Argument::TransferId, transferId ), interfaces );
}



void FileTransferPlugin::sendFinishMessage( QUuid transferId, const QString& fileName,
											bool openFileInApplication, const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage( FeatureMessage( m_fileTransferFeature.uid(), FileTransferFinishCommand ).
						addArgument( Argument::TransferId, transferId ).
						addArgument( Argument::Filename, fileName ).
						addArgument( Argument::OpenFileInApplication, openFileInApplication ), interfaces );
}



void FileTransferPlugin::sendOpenTransferFolderMessage( const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage( FeatureMessage( m_fileTransferFeature.uid(), OpenTransferFolder ), interfaces );
}



QString FileTransferPlugin::destinationDirectory() const
{
	auto dir = VeyonCore::filesystem().expandPath( m_configuration.fileTransferDestinationDirectory() );
	if( QDir( dir ).exists() == false &&
		m_configuration.fileTransferCreateDestinationDirectory() &&
		VeyonCore::filesystem().ensurePathExists( dir ) == false )
	{
		return QDir::homePath();
	}

	return dir;
}



ConfigurationPage* FileTransferPlugin::createConfigurationPage()
{
	return new FileTransferConfigurationPage( m_configuration );
}


IMPLEMENT_CONFIG_PROXY(FileTransferConfiguration)
