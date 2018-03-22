/*
 * DesktopServicesFeaturePlugin.cpp - implementation of DesktopServicesFeaturePlugin class
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

#include <QDesktopServices>
#include <QInputDialog>
#include <QUrl>

#include "Computer.h"
#include "ComputerControlInterface.h"
#include "DesktopServicesConfigurationPage.h"
#include "DesktopServicesFeaturePlugin.h"
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



bool DesktopServicesFeaturePlugin::startMasterFeature( const Feature& feature,
													   const ComputerControlInterfaceList& computerControlInterfaces,
													   QWidget* parent )
{
	if( m_features.contains( feature ) == false )
	{
		return false;
	}

	if( feature == m_runProgramFeature )
	{
		RunProgramDialog runProgramDialog( parent );

		if( runProgramDialog.exec() == QDialog::Accepted &&
				runProgramDialog.programs().isEmpty() == false )
		{
			sendFeatureMessage( FeatureMessage( feature.uid(), FeatureMessage::DefaultCommand ).
								addArgument( ProgramsArgument, runProgramDialog.programs() ), computerControlInterfaces );
		}
	}
	else if( feature == m_openWebsiteFeature )
	{
		QString urlAddress = QInputDialog::getText( parent,
													tr( "Open website" ),
													tr( "Please enter the URL of the website to open:" ) );
		QUrl url( urlAddress, QUrl::TolerantMode );
		if( url.scheme().isEmpty() )
		{
			url = QUrl( QStringLiteral("http://") + urlAddress, QUrl::TolerantMode );
		}

		if( urlAddress.isEmpty() == false && url.isValid() )
		{
			sendFeatureMessage( FeatureMessage( feature.uid(), FeatureMessage::DefaultCommand ).
								addArgument( WebsiteUrlArgument, url ), computerControlInterfaces );
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
							addArgument( ProgramsArgument, predefinedServicePath( feature.uid() ) ), computerControlInterfaces );

	}

	return true;
}



bool DesktopServicesFeaturePlugin::stopMasterFeature( const Feature& feature,
													  const ComputerControlInterfaceList& computerControlInterfaces,
													  QWidget* parent )
{
	Q_UNUSED(feature);
	Q_UNUSED(computerControlInterfaces);
	Q_UNUSED(parent);

	return false;
}



bool DesktopServicesFeaturePlugin::handleMasterFeatureMessage( const FeatureMessage& message,
															   ComputerControlInterface::Pointer computerControlInterface )
{
	Q_UNUSED(message);
	Q_UNUSED(computerControlInterface);

	return false;
}



bool DesktopServicesFeaturePlugin::handleServiceFeatureMessage( const FeatureMessage& message,
																FeatureWorkerManager& featureWorkerManager )
{
	Q_UNUSED(featureWorkerManager);

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
		openWebsite( message.argument( WebsiteUrlArgument ).toUrl() );
	}
	else
	{
		return false;
	}

	return true;
}



bool DesktopServicesFeaturePlugin::handleWorkerFeatureMessage( const FeatureMessage& message )
{
	Q_UNUSED(message);

	return false;
}



ConfigurationPage* DesktopServicesFeaturePlugin::createConfigurationPage()
{
	return new DesktopServicesConfigurationPage( m_configuration );

}



void DesktopServicesFeaturePlugin::runProgramAsUser( const QString& program )
{
	qDebug() << "DesktopServicesFeaturePlugin::runProgramAsUser(): launching program" << program;

	VeyonCore::platform().coreFunctions().runProgramAsUser( program,
															VeyonCore::platform().userFunctions().currentUser(),
															VeyonCore::platform().coreFunctions().activeDesktopName() );
}



void DesktopServicesFeaturePlugin::openWebsite( const QUrl& url )
{
	if( QDesktopServices::openUrl( url ) == false )
	{
		qWarning() << "DesktopServicesFeaturePlugin: could not open URL" << url
				   << "via QDesktopServices - trying native generic URL handler";

		runProgramAsUser( QStringLiteral("%1 %2").arg(
							  VeyonCore::platform().coreFunctions().genericUrlHandler(),
							  url.toString() ) );
	}
}



FeatureList DesktopServicesFeaturePlugin::predefinedPrograms() const
{
	const auto programs = m_configuration.predefinedPrograms();

	FeatureList programFeatures;
	programFeatures.reserve( programs.size() );

	for( const auto program : programs )
	{
		const auto programObject = DesktopServiceObject( program.toObject() );
		programFeatures.append( Feature( Feature::Action | Feature::Master, programObject.uid(), m_runProgramFeature.uid(),
										 programObject.name(), programObject.name(),
										 tr("Run program \"%1\"").arg( programObject.name() ) ) );
	}

	return programFeatures;
}



FeatureList DesktopServicesFeaturePlugin::predefinedWebsites() const
{
	const auto websites = m_configuration.predefinedWebsites();

	FeatureList websiteFeatures;
	websiteFeatures.reserve( websites.size() );

	for( const auto website : websites )
	{
		const auto websiteObject = DesktopServiceObject( website.toObject() );
		websiteFeatures.append( Feature( Feature::Action | Feature::Master, websiteObject.uid(), m_openWebsiteFeature.uid(),
										 websiteObject.name(), websiteObject.name(),
										 tr("Open website \"%1\"").arg( websiteObject.name() ) ) );
	}

	return websiteFeatures;
}



QString DesktopServicesFeaturePlugin::predefinedServicePath( Feature::Uid subFeatureUid ) const
{
	const auto services = m_configuration.predefinedPrograms() + m_configuration.predefinedWebsites();

	for( const auto service : services )
	{
		const auto serviceObject = DesktopServiceObject( service.toObject() );
		if( serviceObject.uid() == subFeatureUid )
		{
			return serviceObject.path();
		}
	}

	return QString();
}
