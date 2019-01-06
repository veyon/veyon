/*
 * AuthKeysConfigurationPage.cpp - implementation of the authentication configuration page
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

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "AuthKeysConfigurationPage.h"
#include "AuthKeysManager.h"
#include "FileSystemBrowser.h"
#include "PlatformUserFunctions.h"
#include "VeyonConfiguration.h"
#include "Configuration/UiMapping.h"

#include "ui_AuthKeysConfigurationPage.h"


AuthKeysConfigurationPage::AuthKeysConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::AuthKeysConfigurationPage),
	m_authKeyTableModel( this ),
	m_keyFilesFilter( tr( "Key files (*.pem)" ) )
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
	CONNECT_BUTTON_SLOT( setAccessGroup );

	reloadKeyTable();

	ui->keyTable->setModel( &m_authKeyTableModel );
}


AuthKeysConfigurationPage::~AuthKeysConfigurationPage()
{
	delete ui;
}



void AuthKeysConfigurationPage::resetWidgets()
{
	FOREACH_VEYON_KEY_AUTHENTICATION_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);

	reloadKeyTable();
}



void AuthKeysConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_VEYON_KEY_AUTHENTICATION_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
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
	const auto keyName = QInputDialog::getText( this, tr( "Authentication key name" ),
												tr( "Please enter the name of the user group or role for which to create an authentication key pair:") );
	if( keyName.isEmpty() == false )
	{
		AuthKeysManager authKeysManager;
		const auto success = authKeysManager.createKeyPair( keyName );

		showResultMessage( success, tr( "Create key pair" ), authKeysManager.resultMessage() );

		reloadKeyTable();
	}
}



void AuthKeysConfigurationPage::deleteKey()
{
	const auto title = ui->deleteKey->text();

	const auto nameAndType = selectedKey().split('/');

	if( nameAndType.size() > 1 )
	{
		const auto name = nameAndType[0];
		const auto type = nameAndType[1];

		if( QMessageBox::question( this, title, tr( "Do you really want to delete authentication key \"%1/%2\"?" ).arg( name, type ) ) ==
				QMessageBox::Yes )
		{
			AuthKeysManager authKeysManager;
			const auto success = authKeysManager.deleteKey( name, type );

			showResultMessage( success, title, authKeysManager.resultMessage() );

			reloadKeyTable();
		}
	}
	else
	{
		showResultMessage( false, title, tr( "Please select a key to delete!" ) );
	}
}



void AuthKeysConfigurationPage::importKey()
{
	const auto title = ui->importKey->text();

	const auto inputFile = QFileDialog::getOpenFileName( this, title, QString(), m_keyFilesFilter );
	if( inputFile.isEmpty() )
	{
		return;
	}

	const auto keyName = QInputDialog::getText( this, tr( "Authentication key name" ),
												tr( "Please enter the name of the user group or role for which to import the authentication key:"),
												QLineEdit::Normal,
												AuthKeysManager::keyNameFromExportedKeyFile( inputFile ) );
	if( keyName.isEmpty() )
	{
		return;
	}

	AuthKeysManager authKeysManager;
	const auto keyType = authKeysManager.detectKeyType( inputFile );
	const auto success = authKeysManager.importKey( keyName, keyType, inputFile );

	showResultMessage( success, title, authKeysManager.resultMessage() );

	reloadKeyTable();
}



void AuthKeysConfigurationPage::exportKey()
{
	const auto title = ui->exportKey->text();

	const auto nameAndType = selectedKey().split('/');

	if( nameAndType.size() > 1 )
	{
		const auto name = nameAndType[0];
		const auto type = nameAndType[1];

		const auto outputFile = QFileDialog::getSaveFileName( this, title, QDir::homePath() + QDir::separator() +
															  AuthKeysManager::exportedKeyFileName( name, type ),
															  m_keyFilesFilter );
		if( outputFile.isEmpty() == false )
		{
			AuthKeysManager authKeysManager;
			const auto success = authKeysManager.exportKey( name, type, outputFile );

			showResultMessage( success, title, authKeysManager.resultMessage() );
		}
	}
	else
	{
		showResultMessage( false, title, tr( "Please select a key to export!" ) );
	}
}



void AuthKeysConfigurationPage::setAccessGroup()
{
	const auto title = ui->setAccessGroup->text();

	const auto key = selectedKey();

	if( key.isEmpty() == false )
	{
		const auto userGroups = VeyonCore::platform().userFunctions().userGroups( VeyonCore::config().domainGroupsForAccessControlEnabled() );
		const auto currentGroup = AuthKeysManager().accessGroup( key );

		bool ok = false;
		const auto selectedGroup = QInputDialog::getItem( this, title,
														  tr( "Please select a user group which to grant access to key \"%1\":" ).arg( key ),
														  userGroups, userGroups.indexOf( currentGroup ), true, &ok );

		if( ok && selectedGroup.isEmpty() == false )
		{
			AuthKeysManager manager;
			const auto success = manager.setAccessGroup( key, selectedGroup );

			showResultMessage( success, title, manager.resultMessage() );

			reloadKeyTable();
		}
	}
	else
	{
		showResultMessage( false, title, tr( "Please select a key which to set the access group for!" ) );
	}
}



void AuthKeysConfigurationPage::reloadKeyTable()
{
	m_authKeyTableModel.reload();
	ui->keyTable->resizeColumnsToContents();
}



QString AuthKeysConfigurationPage::selectedKey() const
{
	const auto row = ui->keyTable->currentIndex().row();
	if( row >= 0 && row < m_authKeyTableModel.rowCount() )
	{
		return m_authKeyTableModel.key( row );
	}

	return QString();
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
