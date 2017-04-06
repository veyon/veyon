/*
 * LocalDataConfigurationPage.cpp - implementation of LocalDataConfigurationPage
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

#include "ItalcCore.h"
#include "ItalcConfiguration.h"
#include "LocalDataConfigurationPage.h"
#include "Configuration/UiMapping.h"

#include "ui_LocalDataConfigurationPage.h"

LocalDataConfigurationPage::LocalDataConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::LocalDataConfigurationPage)
{
	ui->setupUi(this);

#define CONNECT_BUTTON_SLOT(name) \
			connect( ui->name, SIGNAL( clicked() ), this, SLOT( name() ) );
}



LocalDataConfigurationPage::~LocalDataConfigurationPage()
{
	delete ui;
}



void LocalDataConfigurationPage::resetWidgets()
{
}



void LocalDataConfigurationPage::connectWidgetsToProperties()
{
}



void LocalDataConfigurationPage::applyConfiguration()
{
}
