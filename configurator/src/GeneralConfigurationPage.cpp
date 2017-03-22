/*
 * GeneralConfigurationPage.cpp - configuration page with general settings
 *
 * Copyright (c) 2016-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include "GeneralConfigurationPage.h"
#include "FileSystemBrowser.h"
#include "ItalcCore.h"
#include "ItalcConfiguration.h"
#include "LocalSystem.h"
#include "ServiceControl.h"
#include "NetworkObjectDirectory.h"
#include "Configuration/UiMapping.h"
#include "ui_GeneralConfigurationPage.h"


GeneralConfigurationPage::GeneralConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::GeneralConfigurationPage),
	m_pluginManager( this ),
	m_featureManager( m_pluginManager )
{
	ui->setupUi(this);

	ui->applicationName->hide();

	// retrieve list of builtin translations and populate language combobox
	QStringList languages;
	for( auto language : QDir(":/resources/").entryList( QStringList("*.qm") ) )
	{
		QLocale loc(language);
		if( loc.language() == QLocale::C )
		{
			loc = QLocale( QLocale::English );
		}
		languages += QString( "%1 - %2 (%3)" ).arg( QLocale::languageToString(loc.language() ),
													loc.nativeLanguageName(),
													loc.name() );
	}

	std::sort( languages.begin(), languages.end() );

	ui->uiLanguage->addItems( languages );

#ifndef ITALC_BUILD_WIN32
	ui->logToWindowsEventLog->hide();
#endif

#define CONNECT_BUTTON_SLOT(name) \
			connect( ui->name, SIGNAL( clicked() ), this, SLOT( name() ) );

	CONNECT_BUTTON_SLOT( openLogFileDirectory );
	CONNECT_BUTTON_SLOT( clearLogFiles );

	QMap<NetworkObjectDirectory::Backend, QString> backends;
	backends[NetworkObjectDirectory::ConfigurationBackend] = tr( "Local configuration" );
	backends[NetworkObjectDirectory::LdapBackend] = tr( "LDAP" );
	backends[NetworkObjectDirectory::TestBackend] = tr( "Test" );

	ui->networkObjectDirectoryBackend->addItems( backends.values() );
}



GeneralConfigurationPage::~GeneralConfigurationPage()
{
	delete ui;
}



void GeneralConfigurationPage::resetWidgets()
{
	FOREACH_ITALC_UI_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_ITALC_LOGGING_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_ITALC_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);

	m_disabledFeatures = ItalcCore::config().disabledFeatures();

	updateFeatureLists();
}



void GeneralConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_ITALC_UI_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_ITALC_LOGGING_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_ITALC_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void GeneralConfigurationPage::openLogFileDirectory()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).
												exec( ui->logFileDirectory );
}




void GeneralConfigurationPage::clearLogFiles()
{
	bool serviceStopped = false;

	ServiceControl serviceControl( this );

	if( serviceControl.isServiceRunning() )
	{
		if( QMessageBox::question( this, tr( "%1 service" ).arg( ItalcCore::applicationName() ),
				tr( "The %1 service needs to be stopped temporarily "
					"in order to remove the log files. Continue?"
					).arg( ItalcCore::applicationName() ), QMessageBox::Yes | QMessageBox::No,
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
	QDir d( LocalSystem::Path::expand( ItalcCore::config().logFileDirectory() ) );
	foreach( const QString &f, d.entryList( QStringList() << "Italc*.log" ) )
	{
		if( f != "ItalcConfigurator.log" )
		{
			success &= d.remove( f );
		}
	}

#ifdef ITALC_BUILD_WIN32
	d = QDir( "C:\\Windows\\Temp" );
#else
	d = QDir( "/tmp" );
#endif

	foreach( const QString &f, d.entryList( QStringList() << "Italc*.log" ) )
	{
		if( f != "ItalcConfigurator.log" )
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



void GeneralConfigurationPage::enableFeature()
{
	for( auto item : ui->disabledFeaturesListWidget->selectedItems() )
	{
		m_disabledFeatures.removeAll( item->data( Qt::UserRole ).toString() );
	}

	ItalcCore::config().setDisabledFeatures( m_disabledFeatures );

	updateFeatureLists();
}



void GeneralConfigurationPage::disableFeature()
{
	for( auto item : ui->allFeaturesListWidget->selectedItems() )
	{
		QString featureUid = item->data( Qt::UserRole ).toString();
		m_disabledFeatures.removeAll( featureUid );
		m_disabledFeatures.append( featureUid );
	}

	ItalcCore::config().setDisabledFeatures( m_disabledFeatures );

	updateFeatureLists();
}



void GeneralConfigurationPage::updateFeatureLists()
{
	ui->allFeaturesListWidget->setUpdatesEnabled( false );
	ui->disabledFeaturesListWidget->setUpdatesEnabled( false );

	ui->allFeaturesListWidget->clear();
	ui->disabledFeaturesListWidget->clear();

	for( auto feature : m_featureManager.features() )
	{
		if( feature.testFlag( Feature::Builtin ) )
		{
			continue;
		}

		QListWidgetItem* item = new QListWidgetItem( QIcon( feature.iconUrl() ), feature.displayName() );
		item->setData( Qt::UserRole, feature.uid().toString() );

		if( m_disabledFeatures.contains( feature.uid().toString() ) )
		{
			ui->disabledFeaturesListWidget->addItem( item );
		}
		else
		{
			ui->allFeaturesListWidget->addItem( item );
		}
	}

	ui->allFeaturesListWidget->setUpdatesEnabled( true );
	ui->disabledFeaturesListWidget->setUpdatesEnabled( true );
}
