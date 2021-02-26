/*
 * GeneralConfigurationPage.cpp - configuration page with general settings
 *
 * Copyright (c) 2016-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QDir>
#include <QMessageBox>

#include "Configuration/UiMapping.h"
#include "GeneralConfigurationPage.h"
#include "Filesystem.h"
#include "FileSystemBrowser.h"
#include "NetworkObjectDirectoryManager.h"
#include "PasswordDialog.h"
#include "PlatformFilesystemFunctions.h"
#include "PlatformUserFunctions.h"
#include "PluginManager.h"
#include "VeyonServiceControl.h"
#include "VeyonCore.h"
#include "VeyonConfiguration.h"

#include "ui_GeneralConfigurationPage.h"


GeneralConfigurationPage::GeneralConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::GeneralConfigurationPage)
{
	ui->setupUi(this);

	// retrieve list of builtin translations and populate language combobox
	QStringList languages;
	const auto qmFiles = QDir( VeyonCore::translationsDirectory() ).entryList( { QStringLiteral("veyon*.qm") } );

	languages.reserve( qmFiles.count() );

	for( const auto& qmFile : qmFiles )
	{
		QLocale loc( qmFile.split( QLatin1Char('_') ).mid( 1 ).join( QLatin1Char('_') ) );
		if( loc.language() == QLocale::C )
		{
			loc = QLocale( QLocale::English );
		}
		languages += QStringLiteral( "%1 - %2 (%3)" ).arg( QLocale::languageToString( loc.language() ),
														   loc.nativeLanguageName(),
														   loc.name() );
	}

	std::sort( languages.begin(), languages.end() );

	ui->uiLanguage->addItems( languages );

	connect( ui->testAuthenticationButton, &QPushButton::clicked, this, &GeneralConfigurationPage::testAuthentication );
	connect( ui->openLogFileDirectory, &QPushButton::clicked, this, &GeneralConfigurationPage::openLogFileDirectory );
	connect( ui->clearLogFiles, &QPushButton::clicked, this, &GeneralConfigurationPage::clearLogFiles );

	populateNetworkObjectDirectories();
}



GeneralConfigurationPage::~GeneralConfigurationPage()
{
	delete ui;
}



void GeneralConfigurationPage::resetWidgets()
{
	FOREACH_VEYON_UI_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_VEYON_LOGGING_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_VEYON_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void GeneralConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_VEYON_UI_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_VEYON_LOGGING_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_VEYON_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void GeneralConfigurationPage::applyConfiguration()
{
}



void GeneralConfigurationPage::testAuthentication()
{
	switch( VeyonCore::config().authenticationMethod() )
	{
	case VeyonCore::AuthenticationMethod::LogonAuthentication:
		if( testLogonAuthentication() == false )
		{
			return;
		}
		break;
	case VeyonCore::AuthenticationMethod::KeyFileAuthentication:
		if( testKeyFileAuthentication() == false )
		{
			return;
		}
		break;
	}

	QMessageBox::information( this, authenticationTestTitle(),
							  tr( "Authentication is set up properly on this computer." ) );
}



bool GeneralConfigurationPage::testLogonAuthentication()
{
	return VeyonCore::instance()->initAuthentication() &&
			VeyonCore::platform().userFunctions().authenticate( VeyonCore::authenticationCredentials().logonUsername(),
																VeyonCore::authenticationCredentials().logonPassword() );
}



bool GeneralConfigurationPage::testKeyFileAuthentication()
{
	if( VeyonCore::instance()->initAuthentication() == false )
	{
		QMessageBox::critical( this, authenticationTestTitle(),
							   tr( "Authentication keys are not set up properly on this computer." ) );
		return false;
	}

	return true;
}




void GeneralConfigurationPage::openLogFileDirectory()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).
			exec( ui->logFileDirectory );
}




void GeneralConfigurationPage::clearLogFiles()
{
	bool serviceStopped = false;

	VeyonServiceControl serviceControl( this );

	if( serviceControl.isServiceRunning() )
	{
		if( QMessageBox::question( this, tr( "%1 service" ).arg( VeyonCore::applicationName() ),
								   tr( "The %1 service needs to be stopped temporarily "
									   "in order to remove the log files. Continue?"
									   ).arg( VeyonCore::applicationName() ), QMessageBox::Yes | QMessageBox::No,
								   QMessageBox::Yes ) == QMessageBox::Yes )
		{
			serviceControl.stopService();
			serviceStopped = true;
		}
		else
		{
			return;
		}
	}

	bool success = true;
	const QStringList logFilesFilter( { QStringLiteral("Veyon*.log") } );

	QDir d( VeyonCore::filesystem().expandPath( VeyonCore::config().logFileDirectory() ) );
	const auto localLogFiles = d.entryList( logFilesFilter );

	for( const auto& f : localLogFiles )
	{
		if( f.startsWith( QLatin1String("VeyonConfigurator") ) )
		{
			d.remove( f );
		}
		else
		{
			success &= d.remove( f );
		}
	}

	d = QDir( VeyonCore::platform().filesystemFunctions().globalTempPath() );
	const auto globalLogFiles = d.entryList( logFilesFilter );
	for( const auto& f : globalLogFiles )
	{
		if( f != QLatin1String("VeyonConfigurator.log") )
		{
			success &= d.remove( f );
		}
	}

	if( serviceStopped )
	{
		serviceControl.startService();
	}

	if( success )
	{
		QMessageBox::information( this, tr( "Log files cleared" ),
								  tr( "All log files were cleared successfully." ) );
	}
	else
	{
		QMessageBox::critical( this, tr( "Error" ),
							   tr( "Could not remove all log files." ) );
	}
}



void GeneralConfigurationPage::populateNetworkObjectDirectories()
{
	const auto directories = VeyonCore::networkObjectDirectoryManager().availableDirectories();

	for( auto it = directories.constBegin(), end = directories.constEnd(); it != end; ++it )
	{
		ui->networkObjectDirectoryPlugin->addItem( it.value(), it.key() );
	}
}
