/*
 * LinuxPlatformConfigurationPage.cpp - page for configuring service application
 *
 * Copyright (c) 2019 Tobias Junghans <tobydox@veyon.io>
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

#include "VeyonConfiguration.h"
#include "LinuxPlatformConfiguration.h"
#include "LinuxPlatformConfigurationPage.h"
#include "Configuration/UiMapping.h"

#include "ui_LinuxPlatformConfigurationPage.h"


LinuxPlatformConfigurationPage::LinuxPlatformConfigurationPage() :
	ConfigurationPage(),
	ui( new Ui::LinuxPlatformConfigurationPage ),
	m_configuration( &VeyonCore::config() )
{
	ui->setupUi(this);

	Configuration::UiMapping::setFlags( this, Configuration::Property::Flag::Advanced );
}



LinuxPlatformConfigurationPage::~LinuxPlatformConfigurationPage()
{
	delete ui;
}



void LinuxPlatformConfigurationPage::resetWidgets()
{
	FOREACH_LINUX_PLATFORM_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void LinuxPlatformConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_LINUX_PLATFORM_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void LinuxPlatformConfigurationPage::applyConfiguration()
{
}
