/*
 * DemoFeaturePlugin.h - declaration of DemoFeaturePlugin class
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

#ifndef DEMO_FEATURE_PLUGIN_H
#define DEMO_FEATURE_PLUGIN_H

#include "FeaturePluginInterface.h"

class DemoServer;
class DemoClient;

class DemoFeaturePlugin : public QObject, FeaturePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon-solutions.Veyon.Plugins.PluginFeatureInterface")
	Q_INTERFACES(PluginInterface FeaturePluginInterface)
public:
	DemoFeaturePlugin();
	~DemoFeaturePlugin() override;

	Plugin::Uid uid() const override
	{
		return "1b08265b-348f-4978-acaa-45d4f6b90bd9";
	}

	QString version() const override
	{
		return "1.0";
	}

	QString name() const override
	{
		return "Demonstration";
	}

	QString description() const override
	{
		return tr( "Give a demonstration by screen broadcasting" );
	}

	QString vendor() const override
	{
		return "Veyon Community";
	}

	QString copyright() const override
	{
		return "Tobias Doerffel";
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

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
	enum Commands {
		StartDemoServer,
		StopDemoServer,
		StartDemoClient,
		StopDemoClient
	};

	enum Arguments {
		DemoAccessToken,
		VncServerToken,
		IsFullscreenDemo,
		DemoServerHost,
	};

	Feature m_fullscreenDemoFeature;
	Feature m_windowDemoFeature;
	Feature m_demoServerFeature;
	Feature m_demoClientFeature;

	FeatureList m_features;
	QString m_demoAccessToken;
	QString m_demoServerToken;
	QStringList m_demoClientHosts;

	DemoServer* m_demoServer;
	DemoClient* m_demoClient;

};

#endif // DEMO_FEATURE_PLUGIN_H
