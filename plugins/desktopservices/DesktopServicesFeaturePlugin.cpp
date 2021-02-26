/*
 * DesktopServicesFeaturePlugin.cpp - implementation of DesktopServicesFeaturePlugin class
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

#include <QDesktopServices>
#include <QKeyEvent>
#include <QMenu>
#include <QToolButton>
#include <QUrl>

#include "Computer.h"
#include "ComputerControlInterface.h"
#include "DesktopServicesConfigurationPage.h"
#include "DesktopServicesFeaturePlugin.h"
#include "FeatureWorkerManager.h"
#include "ObjectManager.h"
#include "OpenWebsiteDialog.h"
#include "PlatformCoreFunctions.h"
#include "PlatformUserFunctions.h"
#include "RunProgramDialog.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


DesktopServicesFeaturePlugin::DesktopServicesFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_configuration( &VeyonCore::config() ),
	m_runProgramFeature( QStringLiteral( "RunProgram" ),
						 Feature::Action | Feature::AllComponents,
						 Feature::Uid( "da9ca56a-b2ad-4fff-8f8a-929b2927b442" ),
						 Feature::Uid(),
						 tr( "Run program" ), {},
						 tr( "Click this button to run a program on all computers." ),
						 QStringLiteral(":/desktopservices/preferences-desktop-launch-feedback.png") ),
	m_openWebsiteFeature( QStringLiteral( "OpenWebsite" ),
						  Feature::Action | Feature::AllComponents,
						  Feature::Uid( "8a11a75d-b3db-48b6-b9cb-f8422ddd5b0c" ),
						  Feature::Uid(),
						  tr( "Open website" ), {},
						  tr( "Click this button to open a website on all computers." ),
						  QStringLiteral(":/desktopservices/internet-web-browser.png") ),
	m_predefinedProgramsFeatures(),
	m_predefinedWebsitesFeatures(),
	m_features( { m_runProgramFeature, m_openWebsiteFeature } )
{
	connect( VeyonCore::instance(), &VeyonCore::applicationLoaded,
			 this, &DesktopServicesFeaturePlugin::updateFeatures );
}



bool DesktopServicesFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
												  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( operation != Operation::Start )
	{
		return false;
	}

	if( featureUid == m_runProgramFeature.uid() )
	{
		const auto programs = arguments.value( argToString(Argument::Programs) ).toStringList();

		sendFeatureMessage( FeatureMessage{ featureUid, FeatureMessage::DefaultCommand }
								.addArgument( Argument::Programs, programs ),
							computerControlInterfaces );

		return true;
	}

	if( featureUid == m_openWebsiteFeature.uid() )
	{
		const auto websites = arguments.value( argToString(Argument::WebsiteUrl) ).toStringList();

		sendFeatureMessage( FeatureMessage{ featureUid, FeatureMessage::DefaultCommand }
								.addArgument( Argument::WebsiteUrl, websites ),
							computerControlInterfaces );

		return true;
	}

	return false;
}



bool DesktopServicesFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
												 const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature == m_runProgramFeature )
	{
		runProgram( master, computerControlInterfaces );
	}
	else if( feature == m_openWebsiteFeature )
	{
		openWebsite( master, computerControlInterfaces );
	}
	else if( m_predefinedProgramsFeatures.contains( feature ) )
	{
		sendFeatureMessage( FeatureMessage( m_runProgramFeature.uid(), FeatureMessage::DefaultCommand ).
							addArgument( Argument::Programs, predefinedServicePath( feature.uid() ) ), computerControlInterfaces );

	}
	else if( m_predefinedWebsitesFeatures.contains( feature ) )
	{
		sendFeatureMessage( FeatureMessage( m_openWebsiteFeature.uid(), FeatureMessage::DefaultCommand ).
							addArgument( Argument::WebsiteUrl, predefinedServicePath( feature.uid() ) ), computerControlInterfaces );

	}
	else
	{
		return false;
	}

	return true;
}



bool DesktopServicesFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
														 const MessageContext& messageContext,
														 const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( message.featureUid() == m_runProgramFeature.uid() )
	{
		const auto programs = message.argument( Argument::Programs ).toStringList();
		for( const auto& program : programs )
		{
			runProgramAsUser( program );
		}
	}
	else if( message.featureUid() == m_openWebsiteFeature.uid() )
	{
		// forward message to worker running with user privileges
		server.featureWorkerManager().sendMessageToUnmanagedSessionWorker( message );
	}
	else
	{
		return false;
	}

	return true;
}



bool DesktopServicesFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker)

	if( message.featureUid() == m_openWebsiteFeature.uid() )
	{
		openWebsite( message.argument( Argument::WebsiteUrl ).toString() );
		return true;
	}

	return false;
}



ConfigurationPage* DesktopServicesFeaturePlugin::createConfigurationPage()
{
	return new DesktopServicesConfigurationPage( m_configuration );

}



bool DesktopServicesFeaturePlugin::eventFilter( QObject* object, QEvent* event )
{
	auto menu = qobject_cast<QMenu *>( object );
	auto master = VeyonCore::instance()->findChild<VeyonMasterInterface *>();

	if( master != nullptr &&
		menu != nullptr &&
		menu->activeAction() != nullptr &&
		event->type() == QEvent::KeyPress &&
		static_cast<QKeyEvent *>( event )->key() == Qt::Key_Delete )
	{
		DesktopServicesConfiguration userConfig( master->userConfigurationObject() );

		if( menu->objectName() == m_runProgramFeature.name() )
		{
			ObjectManager<DesktopServiceObject> objectManager( userConfig.predefinedPrograms() );
			objectManager.remove( DesktopServiceObject::Uid( menu->activeAction()->objectName() ) );
			userConfig.setPredefinedPrograms( objectManager.objects() );
		}
		else if( menu->objectName() == m_openWebsiteFeature.name() )
		{
			ObjectManager<DesktopServiceObject> objectManager( userConfig.predefinedWebsites() );
			objectManager.remove( DesktopServiceObject::Uid( menu->activeAction()->objectName() ) );
			userConfig.setPredefinedWebsites( objectManager.objects() );
		}

		userConfig.flushStore();

		QTimer::singleShot( 0, this, &DesktopServicesFeaturePlugin::updateFeatures );
		QTimer::singleShot( 0, this, [=]() { openMenu( menu->objectName() ); } );

		return true;
	}

	return QObject::eventFilter( object, event );
}



void DesktopServicesFeaturePlugin::updateFeatures()
{
	updatePredefinedProgramFeatures();
	updatePredefinedWebsiteFeatures();

	m_features = FeatureList( { m_runProgramFeature, m_openWebsiteFeature } ) +
			m_predefinedProgramsFeatures + m_predefinedWebsitesFeatures;

	auto master = VeyonCore::instance()->findChild<VeyonMasterInterface *>();
	if( master )
	{
		master->reloadSubFeatures();

		auto runProgramButton = master->mainWindow()->findChild<QToolButton *>( m_runProgramFeature.name() );
		if( runProgramButton && runProgramButton->menu() )
		{
			runProgramButton->menu()->installEventFilter( this );
		}

		auto openWebsiteButton = master->mainWindow()->findChild<QToolButton *>( m_openWebsiteFeature.name() );
		if( openWebsiteButton && openWebsiteButton->menu() )
		{
			openWebsiteButton->menu()->installEventFilter( this );
		}
	}
}



void DesktopServicesFeaturePlugin::openMenu( const QString& objectName )
{
	auto master = VeyonCore::instance()->findChild<VeyonMasterInterface *>();
	if( master )
	{
		auto button = master->mainWindow()->findChild<QToolButton *>( objectName );
		if( button && button->menu() )
		{
			button->showMenu();
		}
	}
}



void DesktopServicesFeaturePlugin::runProgram( VeyonMasterInterface& master,
											   const ComputerControlInterfaceList& computerControlInterfaces )
{
	RunProgramDialog runProgramDialog( master.mainWindow() );

	if( runProgramDialog.exec() == QDialog::Accepted )
	{
		const auto programs = runProgramDialog.programs().split( QLatin1Char('\n') );

		controlFeature( m_runProgramFeature.uid(), Operation::Start,
						{ { argToString(Argument::Programs), programs } },
						computerControlInterfaces );

		if( runProgramDialog.remember() )
		{
			DesktopServicesConfiguration userConfig( master.userConfigurationObject() );

			ObjectManager<DesktopServiceObject> objectManager( userConfig.predefinedPrograms() );
			objectManager.add( DesktopServiceObject( DesktopServiceObject::Type::Program,
													 runProgramDialog.presetName(),
													 runProgramDialog.programs() ) );
			userConfig.setPredefinedPrograms( objectManager.objects() );
			userConfig.flushStore();

			updateFeatures();
		}
	}
}



void DesktopServicesFeaturePlugin::openWebsite( VeyonMasterInterface& master,
												const ComputerControlInterfaceList& computerControlInterfaces )
{
	OpenWebsiteDialog openWebsiteDialog( master.mainWindow() );

	if( openWebsiteDialog.exec() == QDialog::Accepted )
	{
		controlFeature( m_openWebsiteFeature.uid(), Operation::Start,
						{ { argToString(Argument::WebsiteUrl), openWebsiteDialog.website() } },
						computerControlInterfaces );

		if( openWebsiteDialog.remember() )
		{
			DesktopServicesConfiguration userConfig( master.userConfigurationObject() );

			ObjectManager<DesktopServiceObject> objectManager( userConfig.predefinedWebsites() );
			objectManager.add( DesktopServiceObject( DesktopServiceObject::Type::Website,
													 openWebsiteDialog.presetName(),
													 openWebsiteDialog.website() ) );
			userConfig.setPredefinedWebsites( objectManager.objects() );
			userConfig.flushStore();

			updateFeatures();
		}
	}
}



void DesktopServicesFeaturePlugin::runProgramAsUser( const QString& commandLine )
{
	vDebug() << "launching" << commandLine;

	QString program;
	QStringList parameters;

	// parse command line format "C:\Program Files\..." -foo -bar
	if( commandLine.startsWith( QLatin1Char('"') ) && commandLine.count( QLatin1Char('"') ) > 1 )
	{
		const auto commandLineSplit = commandLine.split( QLatin1Char('"') );
		program = commandLineSplit.value( 1 );
		parameters = commandLine.mid( program.size() + 2 ).split( QLatin1Char(' ') );
	}
	// parse command line format program.exe -foo -bar
	else if( commandLine.contains( QLatin1Char(' ') ) )
	{
		const auto commandLineSplit = commandLine.split( QLatin1Char(' ') );
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
		vWarning() << "empty or invalid URL";
		return false;
	}

	if( QDesktopServices::openUrl( url ) == false )
	{
		vWarning() << "could not open URL" << url << "via QDesktopServices - trying native generic URL handler";

		runProgramAsUser( QStringLiteral("%1 %2").arg(
							  VeyonCore::platform().coreFunctions().genericUrlHandler(),
							  url.toString() ) );
	}

	return true;
}



void DesktopServicesFeaturePlugin::updatePredefinedPrograms()
{
	m_predefinedPrograms = m_configuration.predefinedPrograms();

	auto master = VeyonCore::instance()->findChild<VeyonMasterInterface *>();
	if( master )
	{
		const auto userPrograms = DesktopServicesConfiguration( master->userConfigurationObject() ).predefinedPrograms();
		for( const auto& userProgram : userPrograms )
		{
			m_predefinedPrograms.append( userProgram );
		}
	}
}



void DesktopServicesFeaturePlugin::updatePredefinedWebsites()
{
	m_predefinedWebsites = m_configuration.predefinedWebsites();

	auto master = VeyonCore::instance()->findChild<VeyonMasterInterface *>();
	if( master )
	{
		const auto userWebsites = DesktopServicesConfiguration( master->userConfigurationObject() ).predefinedWebsites();
		for( const auto& userWebsite : userWebsites )
		{
			m_predefinedWebsites.append( userWebsite );
		}
	}
}



void DesktopServicesFeaturePlugin::updatePredefinedProgramFeatures()
{
	m_predefinedProgramsFeatures.clear();

	updatePredefinedPrograms();

	if( m_predefinedPrograms.isEmpty() == false )
	{
		m_predefinedProgramsFeatures.reserve( m_predefinedPrograms.size()+1 );

		for( const auto& program : qAsConst(m_predefinedPrograms) )
		{
			const auto programObject = DesktopServiceObject( program.toObject() );
			m_predefinedProgramsFeatures.append( Feature( m_runProgramFeature.name(), Feature::Action | Feature::Master,
														 programObject.uid(), m_runProgramFeature.uid(),
														 programObject.name(), {},
														 tr("Run program \"%1\"").arg( programObject.path() ) ) );
		}

		auto primaryFeature = m_runProgramFeature;
		primaryFeature.setIconUrl( QStringLiteral(":/core/document-edit.png") );
		primaryFeature.setParentUid( m_runProgramFeature.uid() );
		primaryFeature.setDisplayName( tr("Custom program") );
		m_predefinedProgramsFeatures.append( primaryFeature );
	}
}



void DesktopServicesFeaturePlugin::updatePredefinedWebsiteFeatures()
{
	m_predefinedWebsitesFeatures.clear();

	updatePredefinedWebsites();

	if( m_predefinedWebsites.isEmpty() == false )
	{
		m_predefinedWebsitesFeatures.reserve( m_predefinedWebsites.size()+1 );

		for( const auto& website : qAsConst(m_predefinedWebsites) )
		{
			const auto websiteObject = DesktopServiceObject( website.toObject() );
			m_predefinedWebsitesFeatures.append( Feature( m_openWebsiteFeature.name(), Feature::Action | Feature::Master,
														 websiteObject.uid(), m_openWebsiteFeature.uid(),
														 websiteObject.name(), {},
														 tr("Open website \"%1\"").arg( websiteObject.path() ) ) );
		}

		auto primaryFeature = m_openWebsiteFeature;
		primaryFeature.setIconUrl( QStringLiteral(":/core/document-edit.png") );
		primaryFeature.setParentUid( m_openWebsiteFeature.uid() );
		primaryFeature.setDisplayName( tr("Custom website") );
		m_predefinedWebsitesFeatures.append( primaryFeature );
	}
}



QString DesktopServicesFeaturePlugin::predefinedServicePath( Feature::Uid subFeatureUid ) const
{
	for( const auto& services : { m_predefinedPrograms, m_predefinedWebsites } )
	{
		for( const auto& service : services )
		{
			const auto serviceObject = DesktopServiceObject( service.toObject() );
			if( serviceObject.uid() == subFeatureUid )
			{
				return serviceObject.path();
			}
		}
	}

	return {};
}


IMPLEMENT_CONFIG_PROXY(DesktopServicesConfiguration)
