/*
 * DesktopServicesFeaturePlugin.h - declaration of DesktopServicesFeaturePlugin class
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

#ifndef DESKTOP_SERVICES_FEATURE_PLUGIN_H
#define DESKTOP_SERVICES_FEATURE_PLUGIN_H

#include "ConfigurationPagePluginInterface.h"
#include "DesktopServicesConfiguration.h"
#include "FeatureProviderInterface.h"

class DesktopServicesFeaturePlugin : public QObject, PluginInterface,
		FeatureProviderInterface,
		ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.DesktopServices")
	Q_INTERFACES(PluginInterface
				 FeatureProviderInterface
				 ConfigurationPagePluginInterface)
public:
	DesktopServicesFeaturePlugin( QObject* parent = nullptr );
	~DesktopServicesFeaturePlugin() override {}

	Plugin::Uid uid() const override
	{
		return QStringLiteral("a54ee018-42bf-4569-90c7-0d8470125ccf");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral("DesktopServices");
	}

	QString description() const override
	{
		return tr( "Start programs and services in user desktop" );
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

	bool handleFeatureMessage( VeyonServerInterface& server, const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

	ConfigurationPage* createConfigurationPage() override;

private:
	void runProgramAsUser( const QString& commandLine );
	bool openWebsite( const QString& urlString );

	FeatureList predefinedPrograms() const;
	FeatureList predefinedWebsites() const;

	QString predefinedServicePath( Feature::Uid subFeatureUid ) const;

	enum Arguments {
		ProgramsArgument,
		WebsiteUrlArgument
	};

	DesktopServicesConfiguration m_configuration;

	const Feature m_runProgramFeature;
	const Feature m_openWebsiteFeature;
	const FeatureList m_predefinedProgramsFeatures;
	const FeatureList m_predefinedWebsitesFeatures;
	const FeatureList m_features;

};

#endif // DESKTOP_SERVICES_FEATURE_PLUGIN_H
