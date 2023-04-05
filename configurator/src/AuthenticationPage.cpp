/*
 * AuthenticationPage.cpp - implementation of the AuthenticationPage class
 *
 * Copyright (c) 2020-2023 Tobias Junghans <tobydox@veyon.io>
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
#include <QMessageBox>
#include <QPushButton>

#include "AuthenticationManager.h"
#include "AuthenticationPage.h"
#include "AuthenticationPageTab.h"
#include "VeyonCore.h"

#include "ui_AuthenticationPage.h"

AuthenticationPage::AuthenticationPage( QWidget* parent ) :
	ConfigurationPage( parent ),
	ui(new Ui::AuthenticationPage)
{
	ui->setupUi(this);

	populateTabs();
}



AuthenticationPage::~AuthenticationPage()
{
	delete ui;
}



void AuthenticationPage::resetWidgets()
{
	populateTabs();
	connectWidgetsToProperties();
}



void AuthenticationPage::connectWidgetsToProperties()
{
	for( auto it = m_tabs.constBegin(), end = m_tabs.constEnd(); it != end; ++it )
	{
		const auto plugin = it.key();
		const auto checkBox = it.value()->enabledCheckBox();

		connect( checkBox, &QCheckBox::toggled, this, [plugin, checkBox]() {
			VeyonCore::authenticationManager().setEnabled( plugin, checkBox->isChecked() );
		} );
	}
}



void AuthenticationPage::applyConfiguration()
{
}



void AuthenticationPage::populateTabs()
{
	qDeleteAll(m_tabs);

	const auto plugins = VeyonCore::authenticationManager().plugins();
	for( auto* plugin : plugins )
	{
		if( plugin->authenticationMethodName().isEmpty() )
		{
			continue;
		}

		auto widget = plugin->createAuthenticationConfigurationWidget();

		auto tab = new AuthenticationPageTab;

		tab->enabledCheckBox()->setChecked( VeyonCore::authenticationManager().isEnabled( plugin ) );
		connect( tab->testButton(), &QPushButton::clicked, this, [this, plugin]() {
			if( plugin->initializeCredentials() && plugin->checkCredentials() )
			{
				QMessageBox::information( this, AuthenticationPluginInterface::authenticationTestTitle(),
										  tr( "Authentication is set up properly on this computer." ) );
			}
		} );

		if( widget )
		{
			widget->setEnabled( tab->enabledCheckBox()->isChecked() );
			connect( tab->enabledCheckBox(), &QCheckBox::toggled, widget, &QWidget::setEnabled );

			tab->tabLayout()->insertWidget( 1, widget );
		}


		ui->tabWidget->addTab( tab, plugin->authenticationMethodName() );

		m_tabs[plugin] = tab;
	}
}
