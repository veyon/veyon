/*
 * dialogs.cpp - implementation of dialogs
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "dialogs.h"



aboutDialog::aboutDialog( QWidget * _parent ) :
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





#include "client.h"
#include "classroom_manager.h"
#include "main_window.h"
#include "messagebox.h"


clientSettingsDialog::clientSettingsDialog( client * _client,
						mainWindow * _main_window,
						const QString & _classroom ) :
	QDialog( _main_window ),
	Ui::clientSettings(),
	m_client( _client ),
	m_mainWindow( _main_window )
{
	setupUi( this );

	nameEdit->setFocus();
	int set_to_classroom = 0;

	classroomManager * cm = m_mainWindow->getClassroomManager();
	for( int i = 0; i < cm->m_classRooms.size(); ++i )
	{
		classRoomComboBox->addItem( cm->m_classRooms[i]->text( 0 ) );
		if( _classroom == cm->m_classRooms[i]->text( 0 ) )
		{
			set_to_classroom = i;
		}
	}

	classRoomComboBox->setCurrentIndex( set_to_classroom );

	if( m_client != NULL )
	{
		nameEdit->setText( m_client->name() );
		IPEdit->setText( m_client->localIP() );
		macEdit->setText( m_client->mac() );
		typeComboBox->setCurrentIndex( m_client->type() );
	}
}




void clientSettingsDialog::accept( void )
{
	if( nameEdit->text() == "" )
	{
		messageBox::information( tr( "Missing name" ),
			tr( "You didn't specify a name for the computer!" ),
					QPixmap( ":/resources/stop.png" ) );
		return;
	}
	if( IPEdit->text() == "" )
	{
		messageBox::information( tr( "Missing IP-address/hostname" ),
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
		messageBox::information( tr( "Invalid MAC-address" ),
			tr( "You specified an invalid MAC-address. Either "
				"leave the field blank or enter a valid MAC-"
				"address (use \":\" as separator!)." ),
					QPixmap( ":/resources/stop.png" ) );
		return;
	}

	if( m_client == NULL )
	{
		m_client = new client( IPEdit->text(),
					macEdit->text(),
					nameEdit->text(),
				(client::types) typeComboBox->currentIndex(),
m_mainWindow->getClassroomManager()->m_classRooms[classRoomComboBox->currentIndex()],
					m_mainWindow );
	}
	else
	{
		m_client->setName( nameEdit->text() );
		m_client->setLocalIP( IPEdit->text() );
		m_client->setMac( macEdit->text() );
		m_client->setType( (client::types)
						typeComboBox->currentIndex() );
		m_client->setClassRoom(
m_mainWindow->getClassroomManager()->m_classRooms[classRoomComboBox->currentIndex()] );
		m_client->resetConnection();
	}

	QDialog::accept();
}





textMessageDialog::textMessageDialog( QString & _msg_str, QWidget * _parent ) :
	QDialog( _parent ? _parent->window() : 0 ),
	Ui::textMessage(),
	m_msgStr( _msg_str )
{
	setupUi( this );
}




void textMessageDialog::accept( void )
{
	m_msgStr = textEdit->toPlainText();
	QDialog::accept();
}




remoteLogonDialog::remoteLogonDialog( QWidget * _parent ) :
	QDialog( _parent ? _parent->window() : 0 ),
	Ui::remoteLogon()
{
	setupUi( this );

	domainEdit->setText( __default_domain );

	connect( userNameEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( userNameChanged( const QString & ) ) );
	connect( passwordEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( passwordChanged( const QString & ) ) );
	connect( domainEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( domainChanged( const QString & ) ) );
}




#include "dialogs.moc"

