/*
 * RemoteAccessFeaturePlugin.h - declaration of RemoteAccessFeaturePlugin class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef REMOTE_ACCESS_FEATURE_PLUGIN_H
#define REMOTE_ACCESS_FEATURE_PLUGIN_H

#include "Computer.h"
#include "FeaturePluginInterface.h"
#include "CommandLinePluginInterface.h"


class RemoteAccessFeaturePlugin : public QObject, CommandLinePluginInterface, FeaturePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon.Veyon.Plugins.RemoteAccess")
	Q_INTERFACES(PluginInterface FeaturePluginInterface CommandLinePluginInterface)
public:
	RemoteAccessFeaturePlugin( QObject* parent = nullptr );
	~RemoteAccessFeaturePlugin() override;

	Plugin::Uid uid() const override
	{
		return "387a0c43-1355-4ff6-9e1f-d098e9ce5127";
	}

	QString version() const override
	{
		return QStringLiteral("1.0");
	}

	QString name() const override
	{
		return QStringLiteral("RemoteAccess");
	}

	QString description() const override
	{
		return tr( "Remote view or control a computer" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	const FeatureList& featureList() const override;

	bool startMasterFeature( const Feature& feature,
							 const ComputerControlInterfaceList& computerControlInterfaces,
							 ComputerControlInterface& localComputerControlInterface,
							 QWidget* parent ) override;

	bool handleMasterFeatureMessage( const FeatureMessage& message,
									 ComputerControlInterface& computerControlInterface ) override;

	bool stopMasterFeature( const Feature& feature,
							const ComputerControlInterfaceList& computerControlInterfaces,
							ComputerControlInterface& localComputerControlInterface,
							QWidget* parent ) override;

	bool handleServiceFeatureMessage( const FeatureMessage& message,
									  FeatureWorkerManager& featureWorkerManager ) override;

	bool handleWorkerFeatureMessage( const FeatureMessage& message ) override;

	QString commandLineModuleName() const override
	{
		return QStringLiteral( "remoteaccess" );
	}

	QString commandLineModuleHelp() const override
	{
		return description();
	}

	QStringList commands() const override;

	QString commandHelp( const QString& command ) const override;

	RunResult runCommand( const QStringList& arguments ) override;

private slots:
	CommandLinePluginInterface::RunResult handle_view( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_control( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_help( const QStringList& arguments );

private:
	bool initAuthentication();

	Feature m_remoteViewFeature;
	Feature m_remoteControlFeature;
	FeatureList m_features;
	Computer m_customComputer;
	ComputerControlInterface m_customComputerControlInterface;
	QMap<QString, QString> m_commands;

};

#endif // REMOTE_ACCESS_FEATURE_PLUGIN_H
