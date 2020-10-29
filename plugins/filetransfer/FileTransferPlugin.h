/*
 * FileTransferPlugin.h - declaration of FileTransferPlugin class
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

#pragma once

#include <QFile>
#include <QUrl>

#include "ConfigurationPagePluginInterface.h"
#include "FeatureProviderInterface.h"
#include "FileTransferConfiguration.h"

class FileTransferController;
class FileTransferUserConfiguration;

class FileTransferPlugin : public QObject, FeatureProviderInterface, PluginInterface, ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.FileTransfer")
	Q_INTERFACES(PluginInterface
					  FeatureProviderInterface
						  ConfigurationPagePluginInterface)
	Q_PROPERTY(QString lastFileTransferSourceDirectory READ lastFileTransferSourceDirectory)
public:
	enum class Argument
	{
		TransferId,
		Filename,
		DataChunk,
		OpenFileInApplication,
		OverwriteExistingFile,
		Files
	};
	Q_ENUM(Argument)

	explicit FileTransferPlugin( QObject* parent = nullptr );
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

	bool controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool startFeature( VeyonMasterInterface& master, const Feature& feature,
					   const ComputerControlInterfaceList& computerControlInterfaces ) override;

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

	ConfigurationPage* createConfigurationPage() override;

Q_SIGNALS:
	Q_INVOKABLE void acceptSelectedFiles( const QList<QUrl>& fileUrls );

private:
	const QString& lastFileTransferSourceDirectory() const
	{
		return m_lastFileTransferSourceDirectory;
	}

	QString destinationDirectory() const;

	enum Commands
	{
		FileTransferStartCommand,
		FileTransferContinueCommand,
		FileTransferCancelCommand,
		FileTransferFinishCommand,
		OpenTransferFolder,
		CommandCount
	};

	const Feature m_fileTransferFeature;
	const FeatureList m_features;

	FileTransferConfiguration m_configuration;

	QString m_lastFileTransferSourceDirectory;

	FileTransferController* m_fileTransferController{nullptr};

	QFile m_currentFile{};
	QString m_currentFileName;
	QUuid m_currentTransferId{};

};
