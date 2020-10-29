/*
 * DemoFeaturePlugin.h - declaration of DemoFeaturePlugin class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

#include "AuthenticationPluginInterface.h"
#include "ConfigurationPagePluginInterface.h"
#include "DemoAuthentication.h"
#include "DemoConfiguration.h"
#include "EnumHelper.h"
#include "FeatureProviderInterface.h"

class DemoServer;
class DemoClient;

class DemoFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface, ConfigurationPagePluginInterface, DemoAuthentication
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.Demo")
	Q_INTERFACES(PluginInterface
				 FeatureProviderInterface
				 ConfigurationPagePluginInterface
				 AuthenticationPluginInterface)
public:
	enum class Argument {
		DemoAccessToken,
		VncServerPort,
		VncServerPassword,
		DemoServerHost,
		DemoServerPort
	};
	Q_ENUM(Argument)

	explicit DemoFeaturePlugin( QObject* parent = nullptr );
	~DemoFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("1b08265b-348f-4978-acaa-45d4f6b90bd9");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral("Demonstration");
	}

	QString description() const override
	{
		return tr( "Give a demonstration by screen broadcasting" );
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

	bool stopFeature( VeyonMasterInterface& master, const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

	ConfigurationPage* createConfigurationPage() override;

private:
	static QString a2s( Argument arg )
	{
		return EnumHelper::itemName( arg ).toLower();
	}

	bool controlDemoServer( Operation operation, const QVariantMap& arguments,
						   const ComputerControlInterfaceList& computerControlInterfaces );
	bool controlDemoClient( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						   const ComputerControlInterfaceList& computerControlInterfaces );

	enum Commands {
		StartDemoServer,
		StopDemoServer,
		StartDemoClient,
		StopDemoClient
	};

	const Feature m_demoFeature;
	const Feature m_demoClientFullScreenFeature;
	const Feature m_demoClientWindowFeature;
	const Feature m_shareOwnScreenFullScreenFeature;
	const Feature m_shareOwnScreenWindowFeature;
	const Feature m_shareUserScreenFullScreenFeature;
	const Feature m_shareUserScreenWindowFeature;
	const Feature m_demoServerFeature;
	const FeatureList m_features;

	DemoConfiguration m_configuration;

	QStringList m_demoClientHosts{};

	DemoServer* m_demoServer{nullptr};
	DemoClient* m_demoClient{nullptr};

};
