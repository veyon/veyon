/*
 * AuthKeysConfigurationPage.cpp - implementation of the authentication configuration page
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@users.sf.net>
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

#include <QInputDialog>
#include <QMessageBox>

#include "AuthKeysConfigurationPage.h"
#include "AuthKeysManager.h"
#include "FileSystemBrowser.h"
#include "VeyonConfiguration.h"
#include "Configuration/UiMapping.h"

#include "ui_AuthKeysConfigurationPage.h"


AuthKeysConfigurationPage::AuthKeysConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::AuthKeysConfigurationPage),
	m_keyListModel( this )
{
	ui->setupUi(this);

#define CONNECT_BUTTON_SLOT(name) \
			connect( ui->name, &QAbstractButton::clicked, this, &AuthKeysConfigurationPage::name );

	CONNECT_BUTTON_SLOT( openPublicKeyBaseDir );
	CONNECT_BUTTON_SLOT( openPrivateKeyBaseDir );
	CONNECT_BUTTON_SLOT( createKeyPair );
	CONNECT_BUTTON_SLOT( deleteKey );
	CONNECT_BUTTON_SLOT( importKey );
	CONNECT_BUTTON_SLOT( exportKey );

	ui->keyList->setModel( &m_keyListModel );

	reloadKeyList();
}


AuthKeysConfigurationPage::~AuthKeysConfigurationPage()
{
	delete ui;
}



void AuthKeysConfigurationPage::resetWidgets()
{
	FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void AuthKeysConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void AuthKeysConfigurationPage::applyConfiguration()
{
}



void AuthKeysConfigurationPage::openPublicKeyBaseDir()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).
												exec( ui->publicKeyBaseDir );
}



void AuthKeysConfigurationPage::openPrivateKeyBaseDir()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).
			exec( ui->privateKeyBaseDir );
}



void AuthKeysConfigurationPage::createKeyPair()
{
	const auto keyName = QInputDialog::getText( this, tr( "Key name" ), tr( "Please enter the name of the user group or role for which to create a key pair:") );
	if( keyName.isEmpty() == false )
	{
		AuthKeysManager authKeysManager;
		const auto success = authKeysManager.createKeyPair( keyName );

		showResultMessage( success, tr( "Create key pair" ), authKeysManager.resultMessage() );

		reloadKeyList();
	}
}



void AuthKeysConfigurationPage::deleteKey()
{
	const auto nameAndType = m_keyListModel.data( ui->keyList->currentIndex() ).toString().split('/');
	if( nameAndType.size() > 1 )
	{
		const auto name = nameAndType[0];
		const auto type = nameAndType[1];

		AuthKeysManager authKeysManager;
		const auto success = authKeysManager.deleteKey( name, type );

		showResultMessage( success, tr( "Create key pair" ), authKeysManager.resultMessage() );

		reloadKeyList();
	}
}



void AuthKeysConfigurationPage::importKey()
{

}



void AuthKeysConfigurationPage::exportKey()
{

}



void AuthKeysConfigurationPage::reloadKeyList()
{
	m_keyListModel.setStringList( AuthKeysManager().listKeys() );
}



void AuthKeysConfigurationPage::showResultMessage( bool success, const QString& title, const QString& message )
{
	if( message.isEmpty() )
	{
		return;
	}

	if( success )
	{
		QMessageBox::information( this, title, message );
	}
	else
	{
		QMessageBox::critical( this, title, message );
	}
}
