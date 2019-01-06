/*
 * PowerControlFeaturePlugin.h - declaration of PowerControlFeaturePlugin class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef POWER_CONTROL_FEATURE_PLUGIN_H
#define POWER_CONTROL_FEATURE_PLUGIN_H

#include "Feature.h"
#include "FeatureProviderInterface.h"

class PowerControlFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.PowerControl")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	PowerControlFeaturePlugin( QObject* parent = nullptr );
	~PowerControlFeaturePlugin() override {}

	Plugin::Uid uid() const override
	{
		return QStringLiteral("4122e8ca-b617-4e36-b851-8e050ed2d82e");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
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

	const FeatureList& featureList() const override;

	bool startFeature( VeyonMasterInterface& master, const Feature& feature,
					   const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool stopFeature( VeyonMasterInterface& master, const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
							   ComputerControlInterface::Pointer computerControlInterface ) override;

	bool handleFeatureMessage( VeyonServerInterface& server, const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

private:
	bool confirmFeatureExecution( const Feature& feature, QWidget* parent );
	static void broadcastWOLPacket( QString macAddress );

	const Feature m_powerOnFeature;
	const Feature m_rebootFeature;
	const Feature m_powerDownFeature;
	const FeatureList m_features;

};

#endif // POWER_CONTROL_FEATURE_PLUGIN_H
