/*
 * InternetAccessControlConfigurationPage.cpp - implementation of the access control page
 *
 * Copyright (c) 2016-2018 Tobias Junghans <tobydox@users.sf.net>
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

#include "InternetAccessControlBackendInterface.h"
#include "InternetAccessControlConfiguration.h"
#include "InternetAccessControlConfigurationPage.h"
#include "Configuration/UiMapping.h"
#include "PluginManager.h"

#include "ui_InternetAccessControlConfigurationPage.h"

InternetAccessControlConfigurationPage::InternetAccessControlConfigurationPage( InternetAccessControlConfiguration& configuration,
																				QWidget* parent ) :
	ConfigurationPage( parent ),
	ui( new Ui::InternetAccessControlConfigurationPage ),
	m_configuration( configuration ),
	m_backendConfigurationWidget( nullptr )
{
	ui->setupUi(this);

	populateBackends();
}



InternetAccessControlConfigurationPage::~InternetAccessControlConfigurationPage()
{
	delete ui;
}



void InternetAccessControlConfigurationPage::resetWidgets()
{
	FOREACH_INTERNET_ACCESS_CONTROL_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void InternetAccessControlConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_INTERNET_ACCESS_CONTROL_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}



void InternetAccessControlConfigurationPage::applyConfiguration()
{
}





void InternetAccessControlConfigurationPage::updateBackendConfigurationWidget()
{
	delete m_backendConfigurationWidget;
	m_backendConfigurationWidget = nullptr;

	auto pluginUid = ui->backend->currentData().toUuid();

	auto backendInterface = m_backendInterfaces.value( pluginUid );

	if( backendInterface )
	{
		m_backendConfigurationWidget = backendInterface->configurationWidget();

		if( m_backendConfigurationWidget )
		{
			ui->backendGroupBoxLayout->addWidget( m_backendConfigurationWidget );
		}
	}
}


void InternetAccessControlConfigurationPage::populateBackends()
{
	for( auto pluginObject : qAsConst( VeyonCore::pluginManager().pluginObjects() ) )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto backendInterface = qobject_cast<InternetAccessControlBackendInterface *>( pluginObject );

		if( pluginInterface && backendInterface )
		{
			m_backendInterfaces[pluginInterface->uid()] = backendInterface;
			ui->backend->addItem( pluginInterface->description(), pluginInterface->uid() );
		}
	}
}
