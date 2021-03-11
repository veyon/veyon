/*
 * DesktopServicesFeaturePlugin.h - declaration of DesktopServicesFeaturePlugin class
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "ConfigurationPagePluginInterface.h"
#include "DesktopServicesConfiguration.h"
#include "DesktopServiceObject.h"
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
	enum class Argument
	{
		Applications,
		WebsiteUrl
	};
	Q_ENUM(Argument)

	explicit DesktopServicesFeaturePlugin( QObject* parent = nullptr );
	~DesktopServicesFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("a54ee018-42bf-4569-90c7-0d8470125ccf") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 2, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("DesktopServices");
	}

	QString description() const override
	{
		return tr( "Start apps and open websites in user sessions" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	void upgrade( const QVersionNumber& oldVersion );

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

	ConfigurationPage* createConfigurationPage() override;

Q_SIGNALS:
	Q_INVOKABLE void acceptStartAppDialog( const QString& app, const QString& saveItemName );
	Q_INVOKABLE void acceptOpenWebsiteDialog( const QString& website, const QString& saveItemName );

private:
	bool eventFilter( QObject* object, QEvent* event ) override;

	void updateFeatures();
	void openMenu( const QString& objectName );

	void executeStartAppDialog( VeyonMasterInterface& master,
							 const ComputerControlInterfaceList& computerControlInterfaces );
	void startApp( const QString& app, const QString& saveItemName,
				  VeyonMasterInterface& master, const ComputerControlInterfaceList& computerControlInterfaces );

	void executeOpenWebsiteDialog( VeyonMasterInterface& master,
								  const ComputerControlInterfaceList& computerControlInterfaces );
	void openWebsite( const QString& website, const QString& saveItemName,
					 VeyonMasterInterface& master, const ComputerControlInterfaceList& computerControlInterfaces );

	void runApplicationAsUser( const QString& commandLine );
	bool openWebsite( const QString& urlString );

	void updatePredefinedApplications();
	void updatePredefinedWebsites();

	void updatePredefinedApplicationFeatures();
	void updatePredefinedWebsiteFeatures();

	QString predefinedServicePath( Feature::Uid subFeatureUid ) const;

	DesktopServicesConfiguration m_configuration;

	QJsonArray m_predefinedApps;
	QJsonArray m_predefinedWebsites;

	const Feature m_startAppFeature;
	const Feature m_openWebsiteFeature;

	FeatureList m_predefinedAppsFeatures{};
	FeatureList m_predefinedWebsitesFeatures{};

	FeatureList m_features;

};
