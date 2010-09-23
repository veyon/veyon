/*
 * Dialogs.cpp - implementation of dialogs
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QFile>
#include <QtCore/QRegExp>

#include "Dialogs.h"
#include "Client.h"
#include "ClassroomManager.h"
#include "MainWindow.h"
#include "DecoratedMessageBox.h"

#ifdef ITALC3
#include "GlobalConfig.h"
#include "MasterCore.h"
#include "MasterUI.h"
#endif


AboutDialog::AboutDialog( QWidget * _parent ) :
	QDialog( _parent ? _parent->window() : _parent )
{
	setupUi( this );

	QFile authors_rc( ":/AUTHORS" );
	authors_rc.open( QFile::ReadOnly );
	authors->setPlainText( authors_rc.readAll() );

	QFile license_rc( ":/COPYING" );
	license_rc.open( QFile::ReadOnly );
	license->setPlainText( license_rc.readAll() );
}






ClientSettingsDialog::ClientSettingsDialog( Client * _client,
#ifndef ITALC3
						MainWindow * _main_window,
#endif
						const QString & _classroom ) :
#ifdef ITALC3
	QDialog( MasterUI::mainWindow ),
#else
	QDialog( _main_window ),
#endif
	Ui::ClientSettings(),
	m_client( _client )
#ifndef ITALC3
	,m_mainWindow( _main_window )
#endif
{
	setupUi( this );

	hostnameEdit->setFocus();
	int classroomIndex = 0, i = 0;

#ifdef ITALC3
	ClassroomManager * cm = MasterCore::classroomManager;
	foreach( const Classroom * cr,
			MasterCore::classroomManager->classrooms() )
	{
		classRoomComboBox->addItem( cr->name() );
		if( _classroom == cr->name() )
		{
			classroomIndex = i;
		}
		++i;
	}
#else
	ClassroomManager * cm = m_mainWindow->getClassroomManager();
	for( int i = 0; i < cm->m_classRooms.size(); ++i )
	{
		classRoomComboBox->addItem( cm->m_classRooms[i]->text( 0 ) );
		if( _classroom == cm->m_classRooms[i]->text( 0 ) )
		{
			classroomIndex = i;
		}
	}
#endif

	classRoomComboBox->setCurrentIndex( classroomIndex );

	if( m_client != NULL )
	{
		macEdit->setText( m_client->mac() );
#ifdef ITALC3
		hostnameEdit->setText( m_client->host() );
		nameEdit->setText( m_client->displayName() );
#else
		hostnameEdit->setText( m_client->hostname() );
		nameEdit->setText( m_client->nickname() );
#endif
		typeComboBox->setCurrentIndex( m_client->type() );
	}
}




void ClientSettingsDialog::accept( void )
{
	if( hostnameEdit->text() == "" )
	{
		DecoratedMessageBox::information(
			tr( "Missing IP-address/hostname" ),
			tr( "You didn't specify an IP-address or hostname for "
							"the computer!" ),
					QPixmap( ":/resources/stop.png" ) );
		return;
	}
	// check whether mac-address is valid
	if( macEdit->text() != "" &&
		QString( macEdit->text().toUpper() + ":" ).indexOf(
				QRegExp( "^([\\dA-F]{2}:){6}$" ) ) != 0 )
	{
		DecoratedMessageBox::information(
			tr( "Invalid MAC-address" ),
			tr( "You specified an invalid MAC-address. Either "
				"leave the field blank or enter a valid MAC-"
				"address (use \":\" as separator!)." ),
					QPixmap( ":/resources/stop.png" ) );
		return;
	}

	if( m_client == NULL )
	{
#ifdef ITALC3
		m_client = new Client( hostnameEdit->text(),
					macEdit->text(),
					nameEdit->text(),
				(Client::Types) typeComboBox->currentIndex() );
#else
		m_client = new Client( hostnameEdit->text(),
					macEdit->text(),
					nameEdit->text(),
				(Client::Types) typeComboBox->currentIndex(),
m_mainWindow->getClassroomManager()->m_classRooms[classRoomComboBox->currentIndex()],
					m_mainWindow );
#endif
	}
	else
	{
		m_client->setMac( macEdit->text() );
		m_client->setType( (Client::Types)
                                                typeComboBox->currentIndex() );
#ifdef ITALC3
		m_client->setHost( hostnameEdit->text() );
		m_client->setDisplayName( nameEdit->text() );
		m_client->closeConnection();
		m_client->openConnection();
#else
		m_client->setHostname( hostnameEdit->text() );
		m_client->setNickname( nameEdit->text() );
		m_client->setClassRoom(
m_mainWindow->getClassroomManager()->m_classRooms[classRoomComboBox->currentIndex()] );
		m_client->resetConnection();
#endif
	}

	QDialog::accept();
}





TextMessageDialog::TextMessageDialog( QString & _msg_str, QWidget * _parent ) :
	QDialog( _parent ? _parent->window() : 0 ),
	Ui::TextMessage(),
	m_msgStr( _msg_str )
{
	setupUi( this );
}




void TextMessageDialog::accept( void )
{
	m_msgStr = textEdit->toPlainText();
	QDialog::accept();
}






RemoteLogonDialog::RemoteLogonDialog( QWidget * _parent ) :
	QDialog( _parent ? _parent->window() : 0 ),
	Ui::RemoteLogon()
{
	setupUi( this );

#ifdef ITALC3
	domainEdit->setText( MasterCore::globalConfig->defaultDomain() );
#else
	domainEdit->setText( __default_domain );
#endif
	connect( userNameEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( userNameChanged( const QString & ) ) );
	connect( passwordEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( passwordChanged( const QString & ) ) );
	connect( domainEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( domainChanged( const QString & ) ) );
}



