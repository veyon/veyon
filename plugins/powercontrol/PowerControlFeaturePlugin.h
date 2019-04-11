/*
 * PowerControlFeaturePlugin.h - declaration of PowerControlFeaturePlugin class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "CommandLineIO.h"
#include "CommandLinePluginInterface.h"
#include "Feature.h"
#include "SimpleFeatureProvider.h"

class PowerControlFeaturePlugin : public QObject,
		PluginInterface,
		CommandLineIO,
		CommandLinePluginInterface,
		SimpleFeatureProvider
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.PowerControl")
	Q_INTERFACES(PluginInterface CommandLinePluginInterface FeatureProviderInterface)
public:
	PowerControlFeaturePlugin( QObject* parent = nullptr );
	~PowerControlFeaturePlugin() override {}

	Plugin::Uid uid() const override
	{
		return QStringLiteral("4122e8ca-b617-4e36-b851-8e050ed2d82e");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 2 );
	}

	QString name() const override
	{
		return QStringLiteral("PowerControl");
	}

	QString description() const override
	{
		return tr( "Power on/down or reboot a computer" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	QString commandLineModuleName() const override
	{
		return QStringLiteral( "power" );
	}

	QString commandLineModuleHelp() const override
	{
		return tr( "Commands for controlling power status of computers" );
	}

	QStringList commands() const override;
	QString commandHelp( const QString& command ) const override;

	const FeatureList& featureList() const override;

	bool startFeature( VeyonMasterInterface& master, const Feature& feature,
					   const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

public slots:
	CommandLinePluginInterface::RunResult handle_help( const QStringList& arguments );
	CommandLinePluginInterface::RunResult handle_on( const QStringList& arguments );

private:
	enum Arguments {
		ShutdownTimeout
	};

	bool confirmFeatureExecution( const Feature& feature, QWidget* parent );
	static bool broadcastWOLPacket( QString macAddress );

	void confirmShutdown();
	void displayShutdownTimeout( int shutdownTimeout );

	QMap<QString, QString> m_commands;

	const Feature m_powerOnFeature;
	const Feature m_rebootFeature;
	const Feature m_powerDownFeature;
	const Feature m_powerDownNowFeature;
	const Feature m_installUpdatesAndPowerDownFeature;
	const Feature m_powerDownConfirmedFeature;
	const Feature m_powerDownDelayedFeature;
	const FeatureList m_features;

};
