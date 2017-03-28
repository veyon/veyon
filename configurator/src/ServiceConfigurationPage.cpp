/*
 * ServiceConfigurationPage.cpp - page for configuring service application
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QMessageBox>
#include <QTimer>

#include "ConfiguratorCore.h"
#include "FileSystemBrowser.h"
#include "ItalcConfiguration.h"
#include "PluginManager.h"
#include "ServiceConfigurationPage.h"
#include "ServiceControl.h"
#include "VncServerPluginInterface.h"
#include "Configuration/UiMapping.h"

#include "ui_ServiceConfigurationPage.h"


ServiceConfigurationPage::ServiceConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::ServiceConfigurationPage)
{
	ui->setupUi(this);

#define CONNECT_BUTTON_SLOT(name) \
			connect( ui->name, SIGNAL( clicked() ), this, SLOT( name() ) );

	CONNECT_BUTTON_SLOT( startService );
	CONNECT_BUTTON_SLOT( stopService );

	updateServiceControl();
	populateVncServerPluginComboBox();

	QTimer *serviceUpdateTimer = new QTimer( this );
	serviceUpdateTimer->start( 2000 );

	connect( serviceUpdateTimer, SIGNAL( timeout() ),
				this, SLOT( updateServiceControl() ) );
}



ServiceConfigurationPage::~ServiceConfigurationPage()
{
	delete ui;
}



void ServiceConfigurationPage::resetWidgets()
{
	FOREACH_ITALC_SERVICE_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_ITALC_NETWORK_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_ITALC_VNC_SERVER_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void ServiceConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_ITALC_SERVICE_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_ITALC_NETWORK_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_ITALC_VNC_SERVER_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void ServiceConfigurationPage::startService()
{
	ServiceControl( this ).startService();

	updateServiceControl();
}



void ServiceConfigurationPage::stopService()
{
	ServiceControl( this ).stopService();

	updateServiceControl();
}



void ServiceConfigurationPage::updateServiceControl()
{
	bool running = ServiceControl( this ).isServiceRunning();

#ifdef ITALC_BUILD_WIN32
	ui->startService->setEnabled( !running );
	ui->stopService->setEnabled( running );
#else
	ui->startService->setEnabled( false );
	ui->stopService->setEnabled( false );
#endif
	ui->serviceState->setText( running ? tr( "Running" ) : tr( "Stopped" ) );
}



void ServiceConfigurationPage::populateVncServerPluginComboBox()
{
	PluginManager pluginManager;

	for( auto pluginObject : pluginManager.pluginObjects() )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto vncServerPluginInterface = qobject_cast<VncServerPluginInterface *>( pluginObject );

		if( pluginInterface && vncServerPluginInterface )
		{
			ui->vncServerPlugin->addItem( pluginInterface->description(), pluginInterface->uid() );
		}
	}
}
