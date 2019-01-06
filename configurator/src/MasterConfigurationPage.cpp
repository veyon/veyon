/*
 * MasterConfigurationPage.cpp - page for configuring master application
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

#include "BuiltinFeatures.h"
#include "PluginManager.h"
#include "FeatureManager.h"
#include "FileSystemBrowser.h"
#include "MonitoringMode.h"
#include "VeyonCore.h"
#include "VeyonConfiguration.h"
#include "MasterConfigurationPage.h"
#include "Configuration/UiMapping.h"

#include "ui_MasterConfigurationPage.h"


MasterConfigurationPage::MasterConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::MasterConfigurationPage),
	m_builtinFeatures( new BuiltinFeatures ),
	m_featureManager()
{
	ui->setupUi(this);

#define CONNECT_BUTTON_SLOT(name) \
	connect( ui->name, SIGNAL( clicked() ), this, SLOT( name() ) );

	CONNECT_BUTTON_SLOT( openUserConfigurationDirectory );
	CONNECT_BUTTON_SLOT( openScreenshotDirectory );

	populateFeatureComboBox();
}



MasterConfigurationPage::~MasterConfigurationPage()
{
	delete ui;
	delete m_builtinFeatures;
}



void MasterConfigurationPage::resetWidgets()
{
	FOREACH_VEYON_DIRECTORIES_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_VEYON_MASTER_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);

	m_disabledFeatures = VeyonCore::config().disabledFeatures();

	updateFeatureLists();
}



void MasterConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_VEYON_DIRECTORIES_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_VEYON_MASTER_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void MasterConfigurationPage::applyConfiguration()
{
}



void MasterConfigurationPage::openUserConfigurationDirectory()
{
	FileSystemBrowser( FileSystemBrowser::ExistingFile ).exec( ui->userConfigurationDirectory );
}



void MasterConfigurationPage::openScreenshotDirectory()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).exec( ui->screenshotDirectory );
}



void MasterConfigurationPage::enableFeature()
{
	const auto items = ui->disabledFeaturesListWidget->selectedItems();

	for( auto item : items )
	{
		m_disabledFeatures.removeAll( item->data( Qt::UserRole ).toString() );
	}

	VeyonCore::config().setDisabledFeatures( m_disabledFeatures );

	updateFeatureLists();
}



void MasterConfigurationPage::disableFeature()
{
	const auto items = ui->allFeaturesListWidget->selectedItems();

	for( auto item : items )
	{
		QString featureUid = item->data( Qt::UserRole ).toString();
		m_disabledFeatures.removeAll( featureUid );
		m_disabledFeatures.append( featureUid );
	}

	VeyonCore::config().setDisabledFeatures( m_disabledFeatures );

	updateFeatureLists();
}



void MasterConfigurationPage::populateFeatureComboBox()
{
	ui->computerDoubleClickFeature->addItem( QIcon(), tr( "<no feature>" ), QUuid() );
	ui->computerDoubleClickFeature->insertSeparator( ui->computerDoubleClickFeature->count() );

	for( const auto& feature : m_featureManager.features() )
	{
		if( feature.testFlag( Feature::Master ) )
		{
			ui->computerDoubleClickFeature->addItem( QIcon( feature.iconUrl() ),
													 feature.displayName(),
													 feature.uid() );
		}
	}
}



void MasterConfigurationPage::updateFeatureLists()
{
	ui->allFeaturesListWidget->setUpdatesEnabled( false );
	ui->disabledFeaturesListWidget->setUpdatesEnabled( false );

	ui->allFeaturesListWidget->clear();
	ui->disabledFeaturesListWidget->clear();

	for( const auto& feature : qAsConst( m_featureManager.features() ) )
	{
		if( feature.testFlag( Feature::Master ) == false ||
				feature == m_builtinFeatures->monitoringMode().feature() )
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
