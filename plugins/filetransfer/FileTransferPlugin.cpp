/*
 * FileTransferPlugin.cpp - implementation of FileTransferPlugin class
 *
 * Copyright (c) 2018-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "BuiltinFeatures.h"
#include "Filesystem.h"
#include "FileCollectDialog.h"
#include "FileCollectWorker.h"
#include "FileTransferConfigurationPage.h"
#include "FileTransferController.h"
#include "FileTransferDialog.h"
#include "FileTransferPlugin.h"
#include "FileTransferUserConfiguration.h"
#include "FeatureWorkerManager.h"
#include "PlatformCoreFunctions.h"
#include "PlatformFilesystemFunctions.h"
#include "SystemTrayIcon.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"


FileTransferPlugin::FileTransferPlugin( QObject* parent ) :
	QObject( parent ),
	m_distributeFilesFeature(QStringLiteral("DistributeFiles"),
							 Feature::Flag::Action | Feature::Flag::AllComponents,
							 Feature::Uid("4a70bd5a-fab2-4a4b-a92a-a1e81d2b75ed"),
							 Feature::Uid(),
							 tr("Distribute"), {},
							 tr("Click this button to distribute files from your computer to all computers."),
							 QStringLiteral(":/filetransfer/distribute-files.png") ),
	m_collectFilesFeature(QStringLiteral("FileCollect"),
						  Feature::Flag::Action | Feature::Flag::AllComponents,
						  Feature::Uid("5a14c971-e93c-457f-97a0-0b8f1058a58e"),
						  Feature::Uid(),
						  tr("Collect" ), {},
						  tr("Click this button to collect files from all computers to your computer."),
						  QStringLiteral(":/filetransfer/collect-files.png")),
	m_configuration(&VeyonCore::config())
{
}



FileTransferPlugin::~FileTransferPlugin()
{
	delete m_fileTransferController;
}



bool FileTransferPlugin::controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
										 const ComputerControlInterfaceList& computerControlInterfaces )
{
	if (featureUid == m_distributeFilesFeature.uid())
	{
		return controlDistributeFilesFeature(operation, arguments, computerControlInterfaces);
	}

	if (featureUid == m_collectFilesFeature.uid())
	{
		return controlCollectFilesFeature(operation, arguments, computerControlInterfaces);
	}

	return false;
}



bool FileTransferPlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
									   const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature == m_distributeFilesFeature )
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
				dialog->open();
			}
		}

		return true;
	}

	if (feature == m_collectFilesFeature)
	{
		if (controlFeature(feature.uid(), Operation::Initialize, {}, computerControlInterfaces))
		{
			auto dialog = new FileCollectDialog(m_fileCollectController, master.mainWindow());
			connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
			dialog->exec();
		}

		return true;
	}

	return false;
}



bool FileTransferPlugin::handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface, const FeatureMessage& message)
{
	if (message.featureUid() == m_collectFilesFeature.uid())
	{
		const auto collectionId = message.argument(Argument::CollectionId).toUuid();
		const auto transferId = message.argument(Argument::TransferId).toUuid();

		switch (message.command<FeatureCommand>())
		{
		case FeatureCommand::InitFileCollection:
			m_fileCollectController->initCollection(computerControlInterface,
													collectionId,
													message.argument(Argument::Files).toStringList());
			break;
		case FeatureCommand::StartFileTransfer:
			m_fileCollectController->startFileTransfer(computerControlInterface,
													   collectionId,
													   transferId,
													   message.argument(Argument::FileName).toString(),
													   message.argument(Argument::FileSize).toLongLong());
			break;
		case FeatureCommand::ContinueFileTransfer:
			m_fileCollectController->continueFileTransfer(computerControlInterface,
														  collectionId,
														  message.argument(Argument::TransferId).toUuid(),
														  message.argument(Argument::DataChunk).toByteArray());
			break;
		case FeatureCommand::FinishFileTransfer:
			m_fileCollectController->finishFileTransfer(computerControlInterface,
														collectionId,
														message.argument(Argument::TransferId).toUuid());
			break;
		case FeatureCommand::SkipFileTransfer:
			m_fileCollectController->skipToNextFileTransfer(computerControlInterface, collectionId);
			break;
		case FeatureCommand::RetryFileTransfer:
			m_fileCollectController->retryFileTransfer(computerControlInterface, collectionId);
			break;
		case FeatureCommand::FinishFileCollection:
			m_fileCollectController->finishFileCollection(computerControlInterface, collectionId);
			break;
		default:
			break;
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

	if (message.featureUid() == m_distributeFilesFeature.uid())
	{
		if (message.command<FeatureCommand>() == FeatureCommand::FinishFileTransfer)
		{
			VeyonCore::builtinFeatures().systemTrayIcon().showMessage( m_distributeFilesFeature.displayName(),
																	   tr( "Received file \"%1\"." ).
																	   arg( message.argument( Argument::FileName ).toString() ),
																	   server.featureWorkerManager() );
		}

		// forward message to worker
		server.featureWorkerManager().sendMessageToUnmanagedSessionWorker( message );

		return true;
	}

	if (message.featureUid() == m_collectFilesFeature.uid())
	{
		const auto collectionId = message.argument(Argument::CollectionId).toUuid();

		if (m_fileCollectionContexts.contains(collectionId) == false)
		{
			// don't start session/worker for closing non-existent chat worker
			if (message.command<FeatureCommand>() == FeatureCommand::FinishFileCollection &&
				server.featureWorkerManager().isWorkerRunning(message.featureUid()) == false)
			{
				return true;
			}

			m_fileCollectionContexts[collectionId] = messageContext;
		}

		// forward message to worker
		server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(message);

		if (message.command<FeatureCommand>() == FeatureCommand::FinishFileCollection)
		{
			m_fileCollectionContexts.remove(collectionId);
		}

		return true;
	}

	return false;
}


bool FileTransferPlugin::handleFeatureMessageFromWorker(VeyonServerInterface& server, const FeatureMessage& message)
{
	if (message.featureUid() == m_collectFilesFeature.uid() &&
		message.hasArgument(Argument::CollectionId))
	{
		const auto collectionId = message.argument(Argument::CollectionId).toUuid();

		// forward message to master - if collectionId does not exist, default constructed MessageContext has
		// ioDevice=nullptr so sendFeatureMessageReply() will do nothing
		return server.sendFeatureMessageReply(m_fileCollectionContexts.value(collectionId), message);
	}

	return false;
}



bool FileTransferPlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	if (m_distributeFilesFeature.uid() == message.featureUid())
	{
		return handleDistributeFilesMessage(message);
	}

	if (m_collectFilesFeature.uid() == message.featureUid())
	{
		return handleCollectFilesMessage(worker, message);
	}

	return false;
}



void FileTransferPlugin::sendStartMessage( QUuid transferId, const QString& fileName,
										   bool overwriteExistingFile, const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage(FeatureMessage(m_distributeFilesFeature.uid(), FeatureCommand::StartFileTransfer).
					   addArgument(Argument::TransferId, transferId).
					   addArgument(Argument::FileName, fileName).
					   addArgument(Argument::OverwriteExistingFile, overwriteExistingFile),
					   interfaces);
}



void FileTransferPlugin::sendDataMessage( QUuid transferId, const QByteArray& data,
										  const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage(FeatureMessage(m_distributeFilesFeature.uid(), FeatureCommand::ContinueFileTransfer).
					   addArgument(Argument::TransferId, transferId).
					   addArgument(Argument::DataChunk, data),
					   interfaces);
}



void FileTransferPlugin::sendCancelMessage( QUuid transferId,
											const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage(FeatureMessage( m_distributeFilesFeature.uid(), FeatureCommand::CancelFileTransfer).
					   addArgument(Argument::TransferId, transferId), interfaces);
}



void FileTransferPlugin::sendFinishMessage( QUuid transferId, const QString& fileName,
											bool openFileInApplication, const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage(FeatureMessage(m_distributeFilesFeature.uid(), FeatureCommand::FinishFileTransfer).
					   addArgument(Argument::TransferId, transferId).
					   addArgument(Argument::FileName, fileName).
					   addArgument(Argument::OpenFileInApplication, openFileInApplication), interfaces);
}



void FileTransferPlugin::sendOpenTransferFolderMessage( const ComputerControlInterfaceList& interfaces )
{
	sendFeatureMessage(FeatureMessage(m_distributeFilesFeature.uid(), FeatureCommand::OpenTransferFolder), interfaces);
}



void FileTransferPlugin::sendStopWorkerMessage(const ComputerControlInterfaceList& interfaces)
{
	sendFeatureMessage(FeatureMessage(m_distributeFilesFeature.uid(), FeatureCommand::StopWorker), interfaces);
}



void FileTransferPlugin::sendInitFileCollectionMessage(const FileCollection& collection, ComputerControlInterface::Pointer computerControlInterface)
{
	computerControlInterface->sendFeatureMessage(
				FeatureMessage(m_collectFilesFeature.uid(), FeatureCommand::InitFileCollection)
				.addArgument(Argument::CollectionId, collection.id));
}



void FileTransferPlugin::sendFinishFileCollectionMessage(const FileCollection& collection, ComputerControlInterface::Pointer computerControlInterface)
{
	computerControlInterface->sendFeatureMessage(
				FeatureMessage(m_collectFilesFeature.uid(), FeatureCommand::FinishFileCollection)
				.addArgument(Argument::CollectionId, collection.id));
}



void FileTransferPlugin::sendStartFileTransferMessage(const FileCollection& collection, ComputerControlInterface::Pointer computerControlInterface)
{
	computerControlInterface->sendFeatureMessage(
				FeatureMessage(m_collectFilesFeature.uid(), FeatureCommand::StartFileTransfer)
				.addArgument(Argument::CollectionId, collection.id));
}



void FileTransferPlugin::sendContinueFileTransferMessage(const FileCollection& collection, ComputerControlInterface::Pointer computerControlInterface)
{
	computerControlInterface->sendFeatureMessage(
				FeatureMessage(m_collectFilesFeature.uid(), FeatureCommand::ContinueFileTransfer)
				.addArgument(Argument::CollectionId, collection.id)
				.addArgument(Argument::TransferId, collection.currentTransferId));
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



FileTransferPlugin::LockedFileAction FileTransferPlugin::queryLockedFileAction(const QString& fileName)
{
	qApp->setQuitOnLastWindowClosed(false);

	const auto processId = VeyonCore::platform().filesystemFunctions().findFileLockingProcess(fileName);
	const auto applicationName = VeyonCore::platform().coreFunctions().getApplicationName(processId);

	QMessageBox lockedFileNotification(QMessageBox::Warning,
									   tr("File transfer"),
									   (applicationName.isEmpty() ?
											tr("The file %1 is to be collected, but is still open in an application.").arg(fileName)
										  :
											tr("The file %1 is to be collected, but is still open in the application <b>%2</b>.").arg(fileName, applicationName)
											)
									   + QLatin1Char(' ') +
									   tr("Please save your changes and close the program so that the transfer can be completed.")
									   , QMessageBox::Retry | QMessageBox::Ignore);

	VeyonCore::platform().coreFunctions().raiseWindow(&lockedFileNotification, true);

	if (lockedFileNotification.exec() == QMessageBox::Retry)
	{
		return LockedFileAction::RetryOpeningLockedFile;
	}

	QMessageBox confirmDialog(QMessageBox::Question,
							  m_collectFilesFeature.displayName(),
							  tr("Are you sure you want to skip transferring the file %1?").
							  arg(fileName, QString{}), QMessageBox::Yes | QMessageBox::No);

	VeyonCore::platform().coreFunctions().raiseWindow(&confirmDialog, true);

	if (confirmDialog.exec() == QMessageBox::Yes)
	{
		return LockedFileAction::SkipLockedFile;
	}

	return LockedFileAction::RetryOpeningLockedFile;
}



ConfigurationPage* FileTransferPlugin::createConfigurationPage()
{
	return new FileTransferConfigurationPage( m_configuration );
}



bool FileTransferPlugin::handleDistributeFilesMessage(const FeatureMessage& message)
{
	switch (message.command<FeatureCommand>())
	{
	case FeatureCommand::StartFileTransfer:
		m_currentFile.close();

		m_currentFileName = destinationDirectory() + QDir::separator() + message.argument(Argument::FileName).toString();
		m_currentFile.setFileName(m_currentFileName);
		if( m_currentFile.exists() && message.argument(Argument::OverwriteExistingFile).toBool() == false )
		{
			QMessageBox::critical(nullptr, m_distributeFilesFeature.displayName(),
								  tr("Could not receive file \"%1\" as it already exists.").
								  arg(m_currentFile.fileName()));
			return true;
		}

		if (VeyonCore::platform().filesystemFunctions().openFileSafely(
				&m_currentFile,
				QFile::WriteOnly | QFile::Truncate,
				QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther))
		{
			m_currentTransferId = message.argument(Argument::TransferId).toUuid();
		}
		else
		{
			QMessageBox::critical(nullptr, m_distributeFilesFeature.displayName(),
								  tr("Could not receive file \"%1\" as it could not be opened for writing!").
								  arg(m_currentFile.fileName()));
		}
		return true;

	case FeatureCommand::ContinueFileTransfer:
		if (message.argument(Argument::TransferId).toUuid() == m_currentTransferId)
		{
			m_currentFile.write(message.argument(Argument::DataChunk).toByteArray());
		}
		else
		{
			vWarning() << "received chunk for unknown transfer ID";
		}
		return true;

	case FeatureCommand::CancelFileTransfer:
		if (message.argument(Argument::TransferId).toUuid() == m_currentTransferId)
		{
			m_currentFile.remove();
		}
		else
		{
			vWarning() << "received chunk for unknown transfer ID";
		}
		return true;

	case FeatureCommand::FinishFileTransfer:
		m_currentFile.close();
		if (message.argument(Argument::OpenFileInApplication).toBool())
		{
			QDesktopServices::openUrl( QUrl::fromLocalFile( m_currentFileName ) );
		}
		m_currentFile.setFileName({});
		return true;

	case FeatureCommand::OpenTransferFolder:
		QDesktopServices::openUrl(QUrl::fromLocalFile(destinationDirectory()));
		return true;

	case FeatureCommand::StopWorker:
		QCoreApplication::quit();
		return true;

	default:
		break;
	}

	return false;
}



bool FileTransferPlugin::handleCollectFilesMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	const auto command = message.command<FeatureCommand>();
	const auto collectionId = message.argument(Argument::CollectionId).toUuid();
	const auto transferId = message.argument(Argument::TransferId).toUuid();

	auto fileCollectWorker = m_fileCollectWorkers.value(collectionId);

	FeatureMessage reply(m_collectFilesFeature.uid(), command);
	if (collectionId.isNull() == false)
	{
		reply.addArgument(Argument::CollectionId, collectionId);
	}

	switch (command)
	{
	case FeatureCommand::InitFileCollection:
		if (fileCollectWorker == nullptr)
		{
			fileCollectWorker = new FileCollectWorker(this);
			m_fileCollectWorkers[collectionId] = fileCollectWorker;
		}
		worker.sendFeatureMessageReply(reply.addArgument(Argument::Files, fileCollectWorker->files()));
		return true;

	case FeatureCommand::StartFileTransfer:
		if (fileCollectWorker)
		{
			const auto [state, fileName] = fileCollectWorker->startNextTransfer();
			switch (state)
			{
			case FileCollectWorker::TransferState::Started:
				worker.sendFeatureMessageReply(reply
											   .addArgument(Argument::TransferId, fileCollectWorker->currentTransferId())
											   .addArgument(Argument::FileName, fileCollectWorker->currentFileName())
											   .addArgument(Argument::FileSize, fileCollectWorker->currentFileSize())
											   );
				break;
			case FileCollectWorker::TransferState::AllFinished:
				worker.sendFeatureMessageReply(reply.setCommand(FeatureCommand::FinishFileCollection));
				break;
			case FileCollectWorker::TransferState::WaitingForLockedFile:
				switch (queryLockedFileAction(QDir::toNativeSeparators(fileName)))
				{
				case LockedFileAction::SkipLockedFile:
					fileCollectWorker->skipToNextFile();
					worker.sendFeatureMessageReply(reply.setCommand(FeatureCommand::SkipFileTransfer));
					break;
				case LockedFileAction::RetryOpeningLockedFile:
					worker.sendFeatureMessageReply(reply.setCommand(FeatureCommand::RetryFileTransfer));
					break;
				}
				break;
			}
			return true;
		}
		break;

	case FeatureCommand::ContinueFileTransfer:
		if (fileCollectWorker && fileCollectWorker->currentTransferId() == transferId)
		{
			if (fileCollectWorker->currentFileAtEnd())
			{
				worker.sendFeatureMessageReply(reply
											   .setCommand(FeatureCommand::FinishFileTransfer)
											   .addArgument(Argument::TransferId, transferId));
			}
			else
			{
				worker.sendFeatureMessageReply(reply
											   .addArgument(Argument::TransferId, transferId)
											   .addArgument(Argument::DataChunk, fileCollectWorker->readNextChunk())
											   );
			}
			return true;
		}
		break;

	case FeatureCommand::CancelFileTransfer:
		if (fileCollectWorker)
		{
			fileCollectWorker->cancelCurrentTransfer();
			worker.sendFeatureMessageReply(reply);
			return true;
		}
		break;

	case FeatureCommand::FinishFileCollection:
		if (fileCollectWorker)
		{
			m_fileCollectWorkers.remove(collectionId);
			fileCollectWorker->deleteLater();
			return true;
		}
		break;

	default:
		break;
	}

	vCritical() << "unhandled command" << command << "or invalid file collectionID" << collectionId;

	return false;
}



bool FileTransferPlugin::controlDistributeFilesFeature(Operation operation, const QVariantMap& arguments,
													   const ComputerControlInterfaceList& computerControlInterfaces)
{
	if (operation == Operation::Initialize)
	{
		auto files = arguments.value(argToString(Argument::Files)).toStringList();
		if (files.isEmpty())
		{
			return false;
		}

		for (auto& file : files)
		{
			QFileInfo fileInfo(file);
			if (fileInfo.dir() == QDir::current())
			{
				file = fileInfo.fileName();
			}
		}

		if (m_fileTransferController == nullptr)
		{
			m_fileTransferController = new FileTransferController(this);
		}

		m_fileTransferController->setFiles(files);
		m_fileTransferController->setInterfaces(computerControlInterfaces);

		return true;
	}

	if (operation == Operation::Start)
	{
		if (m_fileTransferController)
		{
			m_fileTransferController->start();
		}

		return true;
	}

	if (operation == Operation::Stop)
	{
		if (m_fileTransferController)
		{
			m_fileTransferController->stop();
		}

		return true;
	}

	return false;
}



bool FileTransferPlugin::controlCollectFilesFeature(Operation operation, const QVariantMap& arguments,
													const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments)

	if (operation == Operation::Initialize)
	{
		if (m_fileCollectController == nullptr)
		{
			m_fileCollectController = new FileCollectController(this);
		}

		m_fileCollectController->setInterfaces(computerControlInterfaces);

		return true;
	}

	if (operation == Operation::Start)
	{
		controlCollectFilesFeature(Operation::Initialize, arguments, computerControlInterfaces);

		if (m_fileCollectController->isRunning())
		{
			vCritical() << "collection already in progress";
			return false;
		}

		m_fileCollectController->start();

		return true;
	}

	if (operation == Operation::Stop)
	{
		if (m_fileCollectController)
		{
			m_fileCollectController->stop();
			m_fileCollectController->deleteLater();
			m_fileCollectController = nullptr;
		}

		return true;
	}

	return false;
}



IMPLEMENT_CONFIG_PROXY(FileTransferConfiguration)
