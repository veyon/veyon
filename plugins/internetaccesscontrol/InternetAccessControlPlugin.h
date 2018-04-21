/*
 * InternetAccessControlPlugin.h - declaration of InternetAccessControlPlugin class
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef INTERNET_ACCESS_CONTROL_PLUGIN_H
#define INTERNET_ACCESS_CONTROL_PLUGIN_H

#include "CommandLinePluginInterface.h"
#include "ConfigurationPagePluginInterface.h"
#include "FeatureProviderInterface.h"
#include "InternetAccessControlConfiguration.h"

class InternetAccessControlBackendInterface;

class InternetAccessControlPlugin : public QObject, PluginInterface,
		FeatureProviderInterface,
		CommandLinePluginInterface,
		ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.InternetAccessControl")
	Q_INTERFACES(PluginInterface
				 FeatureProviderInterface
				 CommandLinePluginInterface
				 ConfigurationPagePluginInterface)
public:
	InternetAccessControlPlugin( QObject* parent = nullptr );
	~InternetAccessControlPlugin() override;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("84a1bbcd-0821-459e-be23-8c1ff69790c8");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral( "InternetAccessControl" );
	}

	QString description() const override
	{
		return tr( "Control access to the internet" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Junghans" );
	}

	QString commandLineModuleName() const override
	{
		return QStringLiteral( "internet" );
	}

	QString commandLineModuleHelp() const override
	{
		return tr( "Commands for controlling access to the internet" );
	}

	QStringList commands() const override;
	QString commandHelp( const QString& command ) const override;

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool startFeature( VeyonMasterInterface& master, const Feature& feature,
					   const ComputerControlInterfaceList& computerControlInterfaces,
					   QWidget* parent ) override;

	bool stopFeature( VeyonMasterInterface& master, const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces,
					  QWidget* parent ) override;

	bool handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
							   ComputerControlInterface::Pointer computerControlInterface ) override;

	bool handleFeatureMessage( VeyonServerInterface& server, const FeatureMessage& message,
							   FeatureWorkerManager& featureWorkerManager ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

	ConfigurationPage* createConfigurationPage() override;


public slots:
	CommandLinePluginInterface::RunResult handle_block( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_unblock( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_help( const QStringList& arguments );

private slots:
	void initBackend();

private:
	InternetAccessControlConfiguration m_configuration;

	InternetAccessControlBackendInterface* m_backendInterface;

	QMap<QString, QString> m_commands;

	Feature m_blockInternetFeature;
	Feature m_unblockInternetFeature;
	FeatureList m_features;

};

#endif // INTERNET_ACCESS_CONTROL_PLUGIN_H
