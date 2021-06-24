/*
 * NetworkObjectDirectoryConfigurationPageTab.cpp - implementation of the NetworkObjectDirectoryConfigurationPageTab class
 *
 * Copyright (c) 2021 Tobias Junghans <tobydox@veyon.io>
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

#include "ConfigurationPage.h"
#include "NetworkObjectDirectoryPluginInterface.h"
#include "NetworkObjectDirectoryConfigurationPageTab.h"

#include "ui_NetworkObjectDirectoryConfigurationPageTab.h"

NetworkObjectDirectoryConfigurationPageTab::NetworkObjectDirectoryConfigurationPageTab(
	NetworkObjectDirectoryPluginInterface* plugin, QWidget* parent ) :
	QWidget( parent ),
	ui(new Ui::NetworkObjectDirectoryConfigurationPageTab),
	m_plugin( plugin ),
	m_content( m_plugin->createNetworkObjectDirectoryConfigurationPage() )
{
	ui->setupUi(this);

	m_content->setEnabled( false );

	connect( ui->enabledCheckBox, &QCheckBox::toggled, m_content, &QWidget::setEnabled );

	ui->tabLayout->insertWidget( 1, m_content );
}



NetworkObjectDirectoryConfigurationPageTab::~NetworkObjectDirectoryConfigurationPageTab()
{
	delete ui;
}



QCheckBox* NetworkObjectDirectoryConfigurationPageTab::enabledCheckBox() const
{
	return ui->enabledCheckBox;
}
