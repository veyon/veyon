/*
 * PowerControlFeaturePlugin.h - declaration of PowerControlFeaturePlugin class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include "FeaturePluginInterface.h"

class PowerControlFeaturePlugin : public QObject, FeaturePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon.Veyon.Plugins.FeaturePluginInterface")
	Q_INTERFACES(PluginInterface FeaturePluginInterface)
public:
	PowerControlFeaturePlugin();
	~PowerControlFeaturePlugin() override {}

	Plugin::Uid uid() const override
	{
		return "4122e8ca-b617-4e36-b851-8e050ed2d82e";
	}

	QString version() const override
	{
		return "1.0";
	}

	QString name() const override
	{
		return "PowerControl";
	}

	QString description() const override
	{
		return tr( "Power on/down or reboot a computer" );
	}

	QString vendor() const override
	{
		return "Veyon Community";
	}

	QString copyright() const override
	{
		return "Tobias Doerffel";
	}

	const FeatureList& featureList() const override;

	bool startMasterFeature( const Feature& feature,
							 const ComputerControlInterfaceList& computerControlInterfaces,
							 ComputerControlInterface& localComputerControlInterface,
							 QWidget* parent ) override;

	bool stopMasterFeature( const Feature& feature,
							const ComputerControlInterfaceList& computerControlInterfaces,
							ComputerControlInterface& localComputerControlInterface,
							QWidget* parent ) override;

	bool handleMasterFeatureMessage( const FeatureMessage& message,
									 ComputerControlInterface& computerControlInterface ) override;

	bool handleServiceFeatureMessage( const FeatureMessage& message,
									  FeatureWorkerManager& featureWorkerManager ) override;

	bool handleWorkerFeatureMessage( const FeatureMessage& message ) override;

private:
	Feature m_powerOnFeature;
	Feature m_rebootFeature;
	Feature m_powerDownFeature;
	FeatureList m_features;

};

#endif // POWER_CONTROL_FEATURE_PLUGIN_H
