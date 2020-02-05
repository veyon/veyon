/*
 * AuthKeysConfigurationDialog.cpp - implementation of the authentication configuration page
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "AuthKeysConfiguration.h"
#include "AuthKeysConfigurationDialog.h"
#include "AuthKeysManager.h"
#include "FileSystemBrowser.h"
#include "PlatformUserFunctions.h"
#include "VeyonConfiguration.h"

#include "ui_AuthKeysConfigurationDialog.h"


AuthKeysConfigurationDialog::AuthKeysConfigurationDialog( AuthKeysConfiguration& configuration,
														  AuthKeysManager& manager ) :
	QDialog( QApplication::activeWindow() ),
	ui( new Ui::AuthKeysConfigurationDialog ),
	m_configuration( configuration ),
	m_manager( manager ),
	m_authKeyTableModel( m_manager, this ),
	m_keyFilesFilter( tr( "Key files (*.pem)" ) )
{
	ui->setupUi(this);

	ui->privateKeyBaseDir->setText( m_configuration.privateKeyBaseDir() );
	ui->publicKeyBaseDir->setText( m_configuration.publicKeyBaseDir() );

#define CONNECT_BUTTON_SLOT(name) \
			connect( ui->name, &QAbstractButton::clicked, this, &AuthKeysConfigurationDialog::name );

	CONNECT_BUTTON_SLOT( openPublicKeyBaseDir )
	CONNECT_BUTTON_SLOT( openPrivateKeyBaseDir )
	CONNECT_BUTTON_SLOT( createKeyPair )
	CONNECT_BUTTON_SLOT( deleteKey )
	CONNECT_BUTTON_SLOT( importKey )
	CONNECT_BUTTON_SLOT( exportKey )
	CONNECT_BUTTON_SLOT( setAccessGroup )

	reloadKeyTable();

	ui->keyTable->setModel( &m_authKeyTableModel );
}



AuthKeysConfigurationDialog::~AuthKeysConfigurationDialog()
{
	delete ui;
}



void AuthKeysConfigurationDialog::accept()
{
	m_configuration.setPrivateKeyBaseDir( ui->privateKeyBaseDir->text() );
	m_configuration.setPublicKeyBaseDir( ui->publicKeyBaseDir->text() );

	QDialog::accept();
}



void AuthKeysConfigurationDialog::openPublicKeyBaseDir()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).
												exec( ui->publicKeyBaseDir );
}



void AuthKeysConfigurationDialog::openPrivateKeyBaseDir()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).
			exec( ui->privateKeyBaseDir );
}



void AuthKeysConfigurationDialog::createKeyPair()
{
	const auto keyName = QInputDialog::getText( this, tr( "Authentication key name" ),
												tr( "Please enter the name of the user group or role for which to create an authentication key pair:") );
	if( keyName.isEmpty() == false )
	{
		const auto success = m_manager.createKeyPair( keyName );

		showResultMessage( success, tr( "Create key pair" ), m_manager.resultMessage() );

		reloadKeyTable();
	}
}



void AuthKeysConfigurationDialog::deleteKey()
{
	const auto title = ui->deleteKey->text();

	const auto nameAndType = selectedKey().split(QLatin1Char('/'));

	if( nameAndType.size() > 1 )
	{
		const auto name = nameAndType[0];
		const auto type = nameAndType[1];

		if( QMessageBox::question( this, title, tr( "Do you really want to delete authentication key \"%1/%2\"?" ).arg( name, type ) ) ==
				QMessageBox::Yes )
		{
			const auto success = m_manager.deleteKey( name, type );

			showResultMessage( success, title, m_manager.resultMessage() );

			reloadKeyTable();
		}
	}
	else
	{
		showResultMessage( false, title, tr( "Please select a key to delete!" ) );
	}
}



void AuthKeysConfigurationDialog::importKey()
{
	const auto title = ui->importKey->text();

	const auto inputFile = QFileDialog::getOpenFileName( this, title, {}, m_keyFilesFilter );
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

	const auto keyType = m_manager.detectKeyType( inputFile );
	const auto success = m_manager.importKey( keyName, keyType, inputFile );

	showResultMessage( success, title, m_manager.resultMessage() );

	reloadKeyTable();
}



void AuthKeysConfigurationDialog::exportKey()
{
	const auto title = ui->exportKey->text();

	const auto nameAndType = selectedKey().split(QLatin1Char('/'));

	if( nameAndType.size() > 1 )
	{
		const auto name = nameAndType[0];
		const auto type = nameAndType[1];

		const auto outputFile = QFileDialog::getSaveFileName( this, title, QDir::homePath() + QDir::separator() +
															  AuthKeysManager::exportedKeyFileName( name, type ),
															  m_keyFilesFilter );
		if( outputFile.isEmpty() == false )
		{
			const auto success = m_manager.exportKey( name, type, outputFile, true );

			showResultMessage( success, title, m_manager.resultMessage() );
		}
	}
	else
	{
		showResultMessage( false, title, tr( "Please select a key to export!" ) );
	}
}



void AuthKeysConfigurationDialog::setAccessGroup()
{
	const auto title = ui->setAccessGroup->text();

	const auto key = selectedKey();

	if( key.isEmpty() == false )
	{
		const auto userGroups = VeyonCore::platform().userFunctions().userGroups( VeyonCore::config().domainGroupsForAccessControlEnabled() );
		const auto currentGroup = m_manager.accessGroup( key );

		bool ok = false;
		const auto selectedGroup = QInputDialog::getItem( this, title,
														  tr( "Please select a user group which to grant access to key \"%1\":" ).arg( key ),
														  userGroups, userGroups.indexOf( currentGroup ), true, &ok );

		if( ok && selectedGroup.isEmpty() == false )
		{
			const auto success = m_manager.setAccessGroup( key, selectedGroup );

			showResultMessage( success, title, m_manager.resultMessage() );

			reloadKeyTable();
		}
	}
	else
	{
		showResultMessage( false, title, tr( "Please select a key which to set the access group for!" ) );
	}
}



void AuthKeysConfigurationDialog::reloadKeyTable()
{
	m_authKeyTableModel.reload();
	ui->keyTable->resizeColumnsToContents();
}



QString AuthKeysConfigurationDialog::selectedKey() const
{
	const auto row = ui->keyTable->currentIndex().row();
	if( row >= 0 && row < m_authKeyTableModel.rowCount() )
	{
		return m_authKeyTableModel.key( row );
	}

	return {};
}



void AuthKeysConfigurationDialog::showResultMessage( bool success, const QString& title, const QString& message )
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
