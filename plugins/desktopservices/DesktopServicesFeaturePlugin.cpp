/*
 * DesktopServicesFeaturePlugin.cpp - implementation of DesktopServicesFeaturePlugin class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#include "VeyonCore.h"

#ifdef VEYON_BUILD_WIN32
#include <windows.h>
#endif

#include <QDesktopServices>
#include <QInputDialog>
#include <QUrl>

#include "Computer.h"
#include "ComputerControlInterface.h"
#include "LocalSystem.h"
#include "DesktopServicesFeaturePlugin.h"
#include "RunProgramDialog.h"
#include "PlatformUserInfoFunctions.h"


DesktopServicesFeaturePlugin::DesktopServicesFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_runProgramFeature( Feature::Action | Feature::AllComponents,
						 Feature::Uid( "da9ca56a-b2ad-4fff-8f8a-929b2927b442" ),
						 tr( "Run program" ), QString(),
						 tr( "Click this button to run a program on all computers." ),
						 QStringLiteral(":/desktopservices/preferences-desktop-launch-feedback.png") ),
	m_openWebsiteFeature( Feature::Action | Feature::AllComponents,
						  Feature::Uid( "8a11a75d-b3db-48b6-b9cb-f8422ddd5b0c" ),
						  tr( "Open website" ), QString(),
						  tr( "Click this button to open a website on all computers." ),
						  QStringLiteral(":/desktopservices/internet-web-browser.png") ),
	m_features( { m_runProgramFeature, m_openWebsiteFeature } )
{
}



bool DesktopServicesFeaturePlugin::startMasterFeature( const Feature& feature,
													   const ComputerControlInterfaceList& computerControlInterfaces,
													   ComputerControlInterface& localComputerControlInterface,
													   QWidget* parent )
{
	Q_UNUSED(localComputerControlInterface);

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

	return true;
}



bool DesktopServicesFeaturePlugin::stopMasterFeature( const Feature& feature,
													  const ComputerControlInterfaceList& computerControlInterfaces,
													  ComputerControlInterface& localComputerControlInterface,
													  QWidget* parent )
{
	Q_UNUSED(feature);
	Q_UNUSED(localComputerControlInterface);
	Q_UNUSED(computerControlInterfaces);
	Q_UNUSED(parent);

	return false;
}



bool DesktopServicesFeaturePlugin::handleMasterFeatureMessage( const FeatureMessage& message,
															   ComputerControlInterface& computerControlInterface )
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



void DesktopServicesFeaturePlugin::runProgramAsUser( const QString& program )
{
	qDebug() << "DesktopServicesFeaturePlugin::runProgramAsUser(): launching program" << program;

	LocalSystem::User user( VeyonCore::platform().userInfoFunctions().loggedOnUser() );
	LocalSystem::Process proc( LocalSystem::Process::findProcessId( QString(), -1, &user ) );
	QString desktop = LocalSystem::Desktop::activeDesktop().name();

#ifdef VEYON_BUILD_WIN32
	LocalSystem::Process::Handle hProcess = proc.runAsUser( program, desktop );
	if( hProcess )
	{
		CloseHandle( hProcess );
	}
#else
	proc.runAsUser( program, desktop );
#endif
}



void DesktopServicesFeaturePlugin::openWebsite( const QUrl& url )
{
	if( QDesktopServices::openUrl( url ) == false )
	{
		qWarning() << "DesktopServicesFeaturePlugin: could not open URL" << url
				   << "via QDesktopServices - trying native approach";
		QString launcher;
#ifdef VEYON_BUILD_LINUX
		launcher = QStringLiteral("xdg-open ");
#elif defined(VEYON_BUILD_WIN32)
		launcher = QStringLiteral("explorer ");
#else
#warning unsupported platform
#endif
		runProgramAsUser( launcher + url.toString() );
	}
}
