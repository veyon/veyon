/*
 * DemoConfigurationPage.cpp - implementation of DemoConfigurationPage
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

#include "DemoConfiguration.h"
#include "DemoConfigurationPage.h"
#include "Configuration/UiMapping.h"

#include "ui_DemoConfigurationPage.h"

DemoConfigurationPage::DemoConfigurationPage( DemoConfiguration& configuration, QWidget* parent ) :
	ConfigurationPage( parent ),
	ui( new Ui::DemoConfigurationPage ),
	m_configuration( configuration )
{
	ui->setupUi(this);

	// MT not supported yet
	ui->multithreadingEnabled->setVisible( false );
}



DemoConfigurationPage::~DemoConfigurationPage()
{
	delete ui;
}



void DemoConfigurationPage::resetWidgets()
{
	// sanitize configuration
	if( m_configuration.framebufferUpdateInterval() < ui->framebufferUpdateInterval->minimum() )
	{
		m_configuration.setFramebufferUpdateInterval( DemoConfiguration::DefaultFramebufferUpdateInterval );
	}

	if( m_configuration.keyFrameInterval() < ui->keyFrameInterval->minimum() )
	{
		m_configuration.setKeyFrameInterval( DemoConfiguration::DefaultKeyFrameInterval );
	}

	if( m_configuration.memoryLimit() < ui->memoryLimit->minimum() )
	{
		m_configuration.setMemoryLimit( DemoConfiguration::DefaultMemoryLimit );
	}

	FOREACH_DEMO_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void DemoConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_DEMO_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}



void DemoConfigurationPage::applyConfiguration()
{
}
