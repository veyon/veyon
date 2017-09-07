/*
 * AuthenticationConfigurationPage.cpp - implementation of the authentication configuration page
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#include <QMessageBox>

#include "AuthenticationConfigurationPage.h"
#include "FileSystemBrowser.h"
#include "VeyonCore.h"
#include "VeyonConfiguration.h"
#include "KeyFileAssistant.h"
#include "LogonAuthentication.h"
#include "PasswordDialog.h"
#include "Configuration/UiMapping.h"

#include "ui_AuthenticationConfigurationPage.h"


AuthenticationConfigurationPage::AuthenticationConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::AuthenticationConfigurationPage)
{
	ui->setupUi(this);

#define CONNECT_BUTTON_SLOT(name) \
			connect( ui->name, SIGNAL( clicked() ), this, SLOT( name() ) );

	CONNECT_BUTTON_SLOT( openPublicKeyBaseDir );
	CONNECT_BUTTON_SLOT( openPrivateKeyBaseDir );

	CONNECT_BUTTON_SLOT( launchKeyFileAssistant );
	CONNECT_BUTTON_SLOT( testLogonAuthentication );
}


AuthenticationConfigurationPage::~AuthenticationConfigurationPage()
{
	delete ui;
}



void AuthenticationConfigurationPage::resetWidgets()
{
	FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void AuthenticationConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void AuthenticationConfigurationPage::applyConfiguration()
{
}



void AuthenticationConfigurationPage::launchKeyFileAssistant()
{
	KeyFileAssistant().exec();
}



void AuthenticationConfigurationPage::openPublicKeyBaseDir()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).
												exec( ui->publicKeyBaseDir );
}



void AuthenticationConfigurationPage::openPrivateKeyBaseDir()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).
												exec( ui->privateKeyBaseDir );
}



void AuthenticationConfigurationPage::testLogonAuthentication()
{
	PasswordDialog dlg( this );
	if( dlg.exec() )
	{
		bool result = LogonAuthentication::authenticateUser( dlg.credentials() );
		if( result )
		{
			QMessageBox::information( this, tr( "Logon authentication test" ),
							tr( "Authentication with provided credentials "
								"was successful." ) );
		}
		else
		{
			QMessageBox::critical( this, tr( "Logon authentication test" ),
							tr( "Authentication with provided credentials "
								"failed!" ) );
		}
	}
}
