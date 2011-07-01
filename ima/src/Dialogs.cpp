/*
 * Dialogs.cpp - implementation of dialogs
 *
 * Copyright (c) 2006-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QRegExp>

#include "Dialogs.h"
#include "Client.h"
#include "ClassroomManager.h"
#include "MainWindow.h"
#include "DecoratedMessageBox.h"

#include "ui_ClientSettingsDialog.h"
#include "ui_TextMessageDialog.h"
#include "ui_SupportDialog.h"
#include "ui_RemoteLogonDialog.h"


#ifdef ITALC3
#include "GlobalConfig.h"
#include "MasterCore.h"
#include "MasterUI.h"
#endif



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
	ui( new Ui::ClientSettingsDialog ),
	m_client( _client )
#ifndef ITALC3
	,m_mainWindow( _main_window )
#endif
{
	ui->setupUi( this );

	ui->hostnameEdit->setFocus();
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
		ui->classRoomComboBox->addItem( cm->m_classRooms[i]->text( 0 ) );
		if( _classroom == cm->m_classRooms[i]->text( 0 ) )
		{
			classroomIndex = i;
		}
	}
#endif

	ui->classRoomComboBox->setCurrentIndex( classroomIndex );

	if( m_client != NULL )
	{
		ui->macEdit->setText( m_client->mac() );
#ifdef ITALC3
		ui->hostnameEdit->setText( m_client->host() );
		ui->nameEdit->setText( m_client->displayName() );
#else
		ui->hostnameEdit->setText( m_client->hostname() );
		ui->nameEdit->setText( m_client->nickname() );
#endif
		ui->typeComboBox->setCurrentIndex( m_client->type() );
	}
}




void ClientSettingsDialog::accept()
{
	if( ui->hostnameEdit->text().isEmpty() )
	{
		DecoratedMessageBox::information(
			tr( "Missing IP address/hostname" ),
			tr( "You didn't specify an IP address or hostname for "
							"the computer!" ),
					QPixmap( ":/resources/stop.png" ) );
		return;
	}

	// check whether mac-address is valid
	if( !ui->macEdit->text().isEmpty() &&
		QString( ui->macEdit->text().toUpper() + ":" ).
					indexOf( QRegExp( "^([\\dA-F]{2}:){6}$" ) ) != 0 )
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
		m_client = new Client( ui->hostnameEdit->text(),
								ui->macEdit->text(),
								ui->nameEdit->text(),
				(Client::Types) ui->typeComboBox->currentIndex() );
#else
		m_client = new Client( ui->hostnameEdit->text(),
								ui->macEdit->text(),
								ui->nameEdit->text(),
				(Client::Types) ui->typeComboBox->currentIndex(),
m_mainWindow->getClassroomManager()->m_classRooms[ui->classRoomComboBox->currentIndex()],
					m_mainWindow );
#endif
	}
	else
	{
		m_client->setMac( ui->macEdit->text() );
		m_client->setType( (Client::Types) ui->typeComboBox->currentIndex() );
#ifdef ITALC3
		m_client->setHost( ui->hostnameEdit->text() );
		m_client->setDisplayName( ui->nameEdit->text() );
		m_client->closeConnection();
		m_client->openConnection();
#else
		m_client->setHostname( ui->hostnameEdit->text() );
		m_client->setNickname( ui->nameEdit->text() );
		m_client->setClassRoom(
m_mainWindow->getClassroomManager()->m_classRooms[ui->classRoomComboBox->currentIndex()] );
		m_client->resetConnection();
#endif
	}

	QDialog::accept();
}





SupportDialog::SupportDialog( QWidget *parent ) :
	QDialog( parent ),
	ui( new Ui::SupportDialog )
{
	ui->setupUi( this );
}



QString SupportDialog::getHost( QWidget *parent )
{
	SupportDialog sd( parent );
	if( sd.exec() == Accepted )
	{
		return sd.ui->hostEdit->text();
	}

	return QString();
}





TextMessageDialog::TextMessageDialog( QString &msgStr, QWidget *parent ) :
	QDialog( parent ),
	ui( new Ui::TextMessageDialog ),
	m_msgStr( msgStr )
{
	ui->setupUi( this );
}




void TextMessageDialog::accept()
{
	m_msgStr = ui->textEdit->toPlainText();
	QDialog::accept();
}






RemoteLogonDialog::RemoteLogonDialog( QWidget *parent ) :
	QDialog( parent ),
	ui( new Ui::RemoteLogonDialog )
{
	ui->setupUi( this );

#ifdef ITALC3
	ui->domainEdit->setText( MasterCore::globalConfig->defaultDomain() );
#else
	ui->domainEdit->setText( __default_domain );
#endif
	connect( ui->userNameEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( userNameChanged( const QString & ) ) );
	connect( ui->passwordEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( passwordChanged( const QString & ) ) );
	connect( ui->domainEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( domainChanged( const QString & ) ) );
}



