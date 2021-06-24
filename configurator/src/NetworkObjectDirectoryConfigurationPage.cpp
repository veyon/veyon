/*
 * NetworkObjectDirectoryConfigurationPage.cpp - implementation of the NetworkObjectDirectoryConfigurationPage class
 *
 * Copyright (c) 2020-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QCheckBox>

#include "Configuration/UiMapping.h"
#include "NetworkObjectDirectoryConfigurationPage.h"
#include "NetworkObjectDirectoryConfigurationPageTab.h"
#include "NetworkObjectDirectoryManager.h"
#include "NetworkObjectDirectoryPluginInterface.h"
#include "VeyonConfiguration.h"

#include "ui_NetworkObjectDirectoryConfigurationPage.h"

NetworkObjectDirectoryConfigurationPage::NetworkObjectDirectoryConfigurationPage( QWidget* parent ) :
	ConfigurationPage( parent ),
	ui(new Ui::NetworkObjectDirectoryConfigurationPage)
{
	ui->setupUi(this);

	populateTabs();
}



NetworkObjectDirectoryConfigurationPage::~NetworkObjectDirectoryConfigurationPage()
{
	delete ui;
}



void NetworkObjectDirectoryConfigurationPage::resetWidgets()
{
	FOREACH_VEYON_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);

	for( const auto* tab : m_tabs )
	{
		tab->content()->resetWidgets();
		tab->enabledCheckBox()->setChecked( VeyonCore::networkObjectDirectoryManager().isEnabled( tab->plugin() ) );

	}
}



void NetworkObjectDirectoryConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_VEYON_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);

	for( const auto* tab : m_tabs )
	{
		const auto plugin = tab->plugin();
		const auto checkBox = tab->enabledCheckBox();

		connect( checkBox, &QCheckBox::toggled, this, [plugin, checkBox]() {
			VeyonCore::networkObjectDirectoryManager().setEnabled( plugin, checkBox->isChecked() );
		} );
	}
}



void NetworkObjectDirectoryConfigurationPage::applyConfiguration()
{
	for( auto tab : m_tabs )
	{
		tab->content()->applyConfiguration();
	}
}



void NetworkObjectDirectoryConfigurationPage::populateTabs()
{
	const auto plugins = VeyonCore::networkObjectDirectoryManager().plugins();
	for( auto* plugin : plugins )
	{
		const auto tab = new NetworkObjectDirectoryConfigurationPageTab( plugin );

		tab->enabledCheckBox()->setChecked( VeyonCore::networkObjectDirectoryManager().isEnabled( plugin ) );

		ui->tabWidget->addTab( tab, tab->content()->windowTitle() );

		m_tabs.append( tab );
	}
}
