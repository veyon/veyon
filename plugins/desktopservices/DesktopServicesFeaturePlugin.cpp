/*
 * DesktopServicesFeaturePlugin.cpp - implementation of DesktopServicesFeaturePlugin class
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

#include <QDesktopServices>
#include <QInputDialog>
#include <QUrl>

#include "Computer.h"
#include "ComputerControlInterface.h"
#include "DesktopServicesConfigurationPage.h"
#include "DesktopServicesFeaturePlugin.h"
#include "VeyonMasterInterface.h"
#include "RunProgramDialog.h"
#include "PlatformCoreFunctions.h"
#include "PlatformUserFunctions.h"


DesktopServicesFeaturePlugin::DesktopServicesFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_configuration(),
	m_runProgramFeature( Feature::Action | Feature::AllComponents,
						 Feature::Uid( "da9ca56a-b2ad-4fff-8f8a-929b2927b442" ),
						 Feature::Uid(),
						 tr( "Run program" ), QString(),
						 tr( "Click this button to run a program on all computers." ),
						 QStringLiteral(":/desktopservices/preferences-desktop-launch-feedback.png") ),
	m_openWebsiteFeature( Feature::Action | Feature::AllComponents,
						  Feature::Uid( "8a11a75d-b3db-48b6-b9cb-f8422ddd5b0c" ),
						  Feature::Uid(),
						  tr( "Open website" ), QString(),
						  tr( "Click this button to open a website on all computers." ),
						  QStringLiteral(":/desktopservices/internet-web-browser.png") ),
	m_predefinedProgramsFeatures( predefinedPrograms() ),
	m_predefinedWebsitesFeatures( predefinedWebsites() ),
	m_features( FeatureList( { m_runProgramFeature, m_openWebsiteFeature } ) + m_predefinedProgramsFeatures + m_predefinedWebsitesFeatures )
{
}



bool DesktopServicesFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
												 const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( m_features.contains( feature ) == false )
	{
		return false;
	}

	if( feature == m_runProgramFeature )
	{
		RunProgramDialog runProgramDialog( master.mainWindow() );

		if( runProgramDialog.exec() == QDialog::Accepted &&
				runProgramDialog.programs().isEmpty() == false )
		{
			sendFeatureMessage( FeatureMessage( feature.uid(), FeatureMessage::DefaultCommand ).
								addArgument( ProgramsArgument, runProgramDialog.programs() ), computerControlInterfaces );
		}
	}
	else if( feature == m_openWebsiteFeature )
	{
		const auto urlString = QInputDialog::getText( master.mainWindow(),
													tr( "Open website" ),
													tr( "Please enter the URL of the website to open:" ) );
		if( urlString.isEmpty() == false )
		{
			sendFeatureMessage( FeatureMessage( feature.uid(), FeatureMessage::DefaultCommand ).
								addArgument( WebsiteUrlArgument, urlString ), computerControlInterfaces );
		}
	}
	else if( m_predefinedProgramsFeatures.contains( feature ) )
	{
		sendFeatureMessage( FeatureMessage( m_runProgramFeature.uid(), FeatureMessage::DefaultCommand ).
							addArgument( ProgramsArgument, predefinedServicePath( feature.uid() ) ), computerControlInterfaces );

	}
	else if( m_predefinedWebsitesFeatures.contains( feature ) )
	{
		sendFeatureMessage( FeatureMessage( m_openWebsiteFeature.uid(), FeatureMessage::DefaultCommand ).
							addArgument( WebsiteUrlArgument, predefinedServicePath( feature.uid() ) ), computerControlInterfaces );

	}

	return true;
}



bool DesktopServicesFeaturePlugin::stopFeature( VeyonMasterInterface& master, const Feature& feature,
												const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(master);
	Q_UNUSED(feature);
	Q_UNUSED(computerControlInterfaces);

	return false;
}



bool DesktopServicesFeaturePlugin::handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
														 ComputerControlInterface::Pointer computerControlInterface )
{
	Q_UNUSED(master);
	Q_UNUSED(message);
	Q_UNUSED(computerControlInterface);

	return false;
}



bool DesktopServicesFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server, const FeatureMessage& message )
{
	Q_UNUSED(server);

	if( message.featureUid() == m_runProgramFeature.uid() )
	{
		const auto programs = message.argument( ProgramsArgument ).toStringList();
		for( const auto& program : programs )
		{
			runProgramAsUser( program );
		}
	}
	else if( message.featureUid() == m_openWebsiteFeature.uid() )
	{
		openWebsite( message.argument( WebsiteUrlArgument ).toString() );
	}
	else
	{
		return false;
	}

	return true;
}



bool DesktopServicesFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker);
	Q_UNUSED(message);

	return false;
}



ConfigurationPage* DesktopServicesFeaturePlugin::createConfigurationPage()
{
	return new DesktopServicesConfigurationPage( m_configuration );

}



void DesktopServicesFeaturePlugin::runProgramAsUser( const QString& commandLine )
{
	qDebug() << "DesktopServicesFeaturePlugin::runProgramAsUser(): launching" << commandLine;

	QString program;
	QStringList parameters;

	// parse command line format "C:\Program Files\..." -foo -bar
	if( commandLine.startsWith( '"' ) && commandLine.count( '"' ) > 1 )
	{
		const auto commandLineSplit = commandLine.split( '"' );
		program = commandLineSplit.value( 1 );
		parameters = commandLine.mid( program.size() + 2 ).split( ' ' );
	}
	// parse command line format program.exe -foo -bar
	else if( commandLine.contains( ' ' ) )
	{
		const auto commandLineSplit = commandLine.split( ' ' );
		program = commandLineSplit.first();
		parameters = commandLineSplit.mid( 1 );
	}
	else
	{
		// no arguments so use command line as program name
		program = commandLine;
	}

	VeyonCore::platform().coreFunctions().runProgramAsUser( program, parameters,
															VeyonCore::platform().userFunctions().currentUser(),
															VeyonCore::platform().coreFunctions().activeDesktopName() );
}



bool DesktopServicesFeaturePlugin::openWebsite( const QString& urlString )
{
	QUrl url( urlString, QUrl::TolerantMode );
	if( url.scheme().isEmpty() )
	{
		url = QUrl( QStringLiteral("http://") + urlString, QUrl::TolerantMode );
	}

	if( url.isEmpty() || url.isValid() == false )
	{
		return false;
	}

	if( QDesktopServices::openUrl( url ) == false )
	{
		qWarning() << "DesktopServicesFeaturePlugin: could not open URL" << url
				   << "via QDesktopServices - trying native generic URL handler";

		runProgramAsUser( QStringLiteral("%1 %2").arg(
							  VeyonCore::platform().coreFunctions().genericUrlHandler(),
							  url.toString() ) );
	}

	return true;
}



FeatureList DesktopServicesFeaturePlugin::predefinedPrograms() const
{
	FeatureList programFeatures;

	const auto programs = m_configuration.predefinedPrograms();

	if( programs.size() > 0 )
	{
		programFeatures.reserve( programs.size() + 1 );

		for( const auto program : programs )
		{
			const auto programObject = DesktopServiceObject( program.toObject() );
			programFeatures.append( Feature( Feature::Action | Feature::Master, programObject.uid(), m_runProgramFeature.uid(),
											 programObject.name(), QString(), tr("Run program \"%1\"").arg( programObject.name() ) ) );
		}

		auto primaryFeature = m_runProgramFeature;
		primaryFeature.setParentUid( m_runProgramFeature.uid() );
		primaryFeature.setDisplayName( tr("Custom program") );
		programFeatures.append( primaryFeature );
	}

	return programFeatures;
}



FeatureList DesktopServicesFeaturePlugin::predefinedWebsites() const
{
	FeatureList websiteFeatures;

	const auto websites = m_configuration.predefinedWebsites();

	if( websites.size() > 0 )
	{
		websiteFeatures.reserve( websites.size() + 1 );

		for( const auto website : websites )
		{
			const auto websiteObject = DesktopServiceObject( website.toObject() );
			websiteFeatures.append( Feature( Feature::Action | Feature::Master, websiteObject.uid(), m_openWebsiteFeature.uid(),
											 websiteObject.name(), QString(),
											 tr("Open website \"%1\"").arg( websiteObject.name() ) ) );
		}

		auto primaryFeature = m_openWebsiteFeature;
		primaryFeature.setParentUid( m_openWebsiteFeature.uid() );
		primaryFeature.setDisplayName( tr("Custom website") );
		websiteFeatures.append( primaryFeature );
	}

	return websiteFeatures;
}



QString DesktopServicesFeaturePlugin::predefinedServicePath( Feature::Uid subFeatureUid ) const
{
	for( const auto& services : { m_configuration.predefinedPrograms(), m_configuration.predefinedWebsites() } )
	{
		for( const auto service : services )
		{
			const auto serviceObject = DesktopServiceObject( service.toObject() );
			if( serviceObject.uid() == subFeatureUid )
			{
				return serviceObject.path();
			}
		}
	}

	return QString();
}
