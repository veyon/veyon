/*
 * DesktopServicesFeaturePlugin.h - declaration of DesktopServicesFeaturePlugin class
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

#ifndef DESKTOP_SERVICES_FEATURE_PLUGIN_H
#define DESKTOP_SERVICES_FEATURE_PLUGIN_H

#include "Feature.h"
#include "FeaturePluginInterface.h"

class DesktopServicesFeaturePlugin : public QObject, FeaturePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon.Veyon.Plugins.FeaturePluginInterface")
	Q_INTERFACES(PluginInterface FeaturePluginInterface)
public:
	DesktopServicesFeaturePlugin( QObject* parent = nullptr );
	~DesktopServicesFeaturePlugin() override {}

	Plugin::Uid uid() const override
	{
		return "a54ee018-42bf-4569-90c7-0d8470125ccf";
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
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

	bool startMasterFeature( const Feature& feature,
							 const ComputerControlInterfaceList& computerControlInterfaces,
							 QWidget* parent ) override;

	bool stopMasterFeature( const Feature& feature,
							const ComputerControlInterfaceList& computerControlInterfaces,
							QWidget* parent ) override;

	bool handleMasterFeatureMessage( const FeatureMessage& message,
									 ComputerControlInterface::Pointer computerControlInterface ) override;

	bool handleServiceFeatureMessage( const FeatureMessage& message,
									  FeatureWorkerManager& featureWorkerManager ) override;

	bool handleWorkerFeatureMessage( const FeatureMessage& message ) override;

private:
	void runProgramAsUser( const QString& program );
	void openWebsite( const QUrl& url );

	enum Arguments {
		ProgramsArgument,
		WebsiteUrlArgument
	};

	Feature m_runProgramFeature;
	Feature m_openWebsiteFeature;
	FeatureList m_features;

};

#endif // DESKTOP_SERVICES_FEATURE_PLUGIN_H
