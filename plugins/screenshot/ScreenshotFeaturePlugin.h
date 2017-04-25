/*
 * ScreenshotFeaturePlugin.h - declaration of ScreenshotFeature class
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

#ifndef SCREENSHOT_FEATURE_PLUGIN_H
#define SCREENSHOT_FEATURE_PLUGIN_H

#include "Feature.h"
#include "FeaturePluginInterface.h"

class ScreenshotFeaturePlugin : public QObject, FeaturePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon.Veyon.Plugins.FeaturePluginInterface")
	Q_INTERFACES(PluginInterface FeaturePluginInterface)
public:
	ScreenshotFeaturePlugin();
	~ScreenshotFeaturePlugin() override {}

	Plugin::Uid uid() const override
	{
		return "ee322521-f4fb-482d-b082-82a79003afa7";
	}

	QString version() const override
	{
		return "1.0";
	}

	QString name() const override
	{
		return "Screenshot";
	}

	QString description() const override
	{
		return tr( "Take screenshots of computers and save them locally." );
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
	Feature m_screenshotFeature;
	FeatureList m_features;

};

#endif // SCREENSHOT_FEATURE_PLUGIN_H
