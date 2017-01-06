/*
 * MasterConfigurationPage.cpp - page for configuring master application
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

#include "FileSystemBrowser.h"
#include "ItalcCore.h"
#include "ItalcConfiguration.h"
#include "MasterConfigurationPage.h"
#include "Configuration/UiMapping.h"

#include "ui_MasterConfigurationPage.h"


MasterConfigurationPage::MasterConfigurationPage() :
    ConfigurationPage(),
	ui(new Ui::MasterConfigurationPage)
{
	ui->setupUi(this);

#define CONNECT_BUTTON_SLOT(name) \
			connect( ui->name, SIGNAL( clicked() ), this, SLOT( name() ) );

	CONNECT_BUTTON_SLOT( openGlobalConfig );
	CONNECT_BUTTON_SLOT( openPersonalConfig );
	CONNECT_BUTTON_SLOT( openSnapshotDirectory );
}



MasterConfigurationPage::~MasterConfigurationPage()
{
	delete ui;
}



void MasterConfigurationPage::resetWidgets()
{
	FOREACH_ITALC_CONFIG_FILE_PATHS_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_ITALC_DATA_DIRECTORIES_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void MasterConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_ITALC_CONFIG_FILE_PATHS_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_ITALC_DATA_DIRECTORIES_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void MasterConfigurationPage::openGlobalConfig()
{
	FileSystemBrowser( FileSystemBrowser::ExistingFile ).exec( ui->globalConfigurationPath );
}




void MasterConfigurationPage::openPersonalConfig()
{
	FileSystemBrowser( FileSystemBrowser::ExistingFile ).exec( ui->personalConfigurationPath );
}



void MasterConfigurationPage::openSnapshotDirectory()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).exec( ui->snapshotDirectory );
}
