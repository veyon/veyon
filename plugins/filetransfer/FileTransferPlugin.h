/*
 * FileTransferPlugin.h - declaration of FileTransferPlugin class
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

#include "FeatureProviderInterface.h"

class FileTransferController;

class FileTransferPlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.FileTransfer")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	FileTransferPlugin( QObject* parent = nullptr );
	~FileTransferPlugin() override;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("d4bb9c42-9eef-4ecb-8dd5-dfd84b355481");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("FileTransfer");
	}

	QString description() const override
	{
		return tr( "Transfer files to remote computer" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool startFeature( VeyonMasterInterface& master, const Feature& feature,
					   const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool stopFeature( VeyonMasterInterface& master, const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
							   ComputerControlInterface::Pointer computerControlInterface ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

	void sendStartMessage( QUuid transferId, const QString& fileName,
						   bool overwriteExistingFile, const ComputerControlInterfaceList& interfaces );
	void sendDataMessage( QUuid transferId, const QByteArray& data, const ComputerControlInterfaceList& interfaces );
	void sendCancelMessage( QUuid transferId, const ComputerControlInterfaceList& interfaces );
	void sendFinishMessage( QUuid transferId, const QString& fileName,
							bool openFileInApplication, const ComputerControlInterfaceList& interfaces );
	void sendOpenTransferFolderMessage( const ComputerControlInterfaceList& interfaces );

private:
	enum Commands
	{
		FileTransferStartCommand,
		FileTransferContinueCommand,
		FileTransferCancelCommand,
		FileTransferFinishCommand,
		OpenTransferFolder,
		CommandCount
	};

	enum Arguments
	{
		TransferId,
		Filename,
		DataChunk,
		OpenFileInApplication,
		OverwriteExistingFile,
		ArgumentsCount
	};

	const Feature m_fileTransferFeature;
	const FeatureList m_features;

	FileTransferController* m_fileTransferController;

	QFile m_currentFile;
	QUuid m_currentTransferId;

};
