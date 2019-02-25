/*
 * ServiceConfigurationPage.cpp - page for configuring service application
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QMessageBox>
#include <QTimer>

#include "FileSystemBrowser.h"
#include "VeyonConfiguration.h"
#include "PluginManager.h"
#include "ServiceConfigurationPage.h"
#include "VeyonServiceControl.h"
#include "VncServerPluginInterface.h"
#include "Configuration/UiMapping.h"

#include "ui_ServiceConfigurationPage.h"


ServiceConfigurationPage::ServiceConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::ServiceConfigurationPage),
	m_vncServerPluginConfigurationWidget( nullptr )
{
	ui->setupUi(this);

	Configuration::UiMapping::setFlags( ui->networkGroupBox, Configuration::Property::Flag::Advanced );

	// TODO: finish multi session support
	ui->isMultiSessionServiceEnabled->hide();

	updateServiceControl();
	populateVncServerPluginComboBox();

	auto serviceUpdateTimer = new QTimer( this );
	serviceUpdateTimer->start( 2000 );

	connect( serviceUpdateTimer, &QTimer::timeout, this, &ServiceConfigurationPage::updateServiceControl );
}



ServiceConfigurationPage::~ServiceConfigurationPage()
{
	delete ui;
}



void ServiceConfigurationPage::resetWidgets()
{
	FOREACH_VEYON_SERVICE_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_VEYON_NETWORK_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_VEYON_VNC_SERVER_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void ServiceConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_VEYON_SERVICE_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_VEYON_NETWORK_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_VEYON_VNC_SERVER_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}


void ServiceConfigurationPage::applyConfiguration()
{
	VeyonServiceControl serviceControl( this );

	if( serviceControl.isServiceRunning() &&
		QMessageBox::question( this, tr( "Restart %1 Service" ).arg( VeyonCore::applicationName() ),
			tr( "All settings were saved successfully. In order to take "
				"effect the %1 service needs to be restarted. "
				"Restart it now?" ).arg( VeyonCore::applicationName() ),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes ) == QMessageBox::Yes )
	{
		serviceControl.stopService();
		serviceControl.startService();
	}
}



void ServiceConfigurationPage::startService()
{
	VeyonServiceControl( this ).startService();

	updateServiceControl();
}



void ServiceConfigurationPage::stopService()
{
	VeyonServiceControl( this ).stopService();

	updateServiceControl();
}



void ServiceConfigurationPage::updateServiceControl()
{
	bool running = VeyonServiceControl( this ).isServiceRunning();

	ui->startService->setEnabled( !running );
	ui->stopService->setEnabled( running );
	ui->serviceState->setText( running ? tr( "Running" ) : tr( "Stopped" ) );
}



void ServiceConfigurationPage::updateVncServerPluginConfigurationWidget()
{
	delete m_vncServerPluginConfigurationWidget;
	m_vncServerPluginConfigurationWidget = nullptr;

	auto pluginUid = ui->vncServerPlugin->currentData().toUuid();

	auto vncServerPluginInterface = m_vncServerPluginInterfaces.value( pluginUid );

	if( vncServerPluginInterface )
	{
		m_vncServerPluginConfigurationWidget = vncServerPluginInterface->configurationWidget();

		if( m_vncServerPluginConfigurationWidget )
		{
			ui->vncServerGroupBoxLayout->addWidget( m_vncServerPluginConfigurationWidget );
		}
	}

	emit widgetsChanged();
}



void ServiceConfigurationPage::populateVncServerPluginComboBox()
{
	for( auto pluginObject : qAsConst( VeyonCore::pluginManager().pluginObjects() ) )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto vncServerPluginInterface = qobject_cast<VncServerPluginInterface *>( pluginObject );

		if( pluginInterface && vncServerPluginInterface )
		{
			m_vncServerPluginInterfaces[pluginInterface->uid()] = vncServerPluginInterface;
			ui->vncServerPlugin->addItem( pluginInterface->description(), pluginInterface->uid() );
		}
	}
}
