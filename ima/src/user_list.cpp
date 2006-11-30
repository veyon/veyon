/*
 * user_list.cpp - implementation of user-list for side-bar
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include <QtGui/QFileDialog>
#include <QtGui/QListWidget>
#include <QtGui/QMenu>
#include <QtGui/QPushButton>
#include <QtGui/QCursor>
#include <QtGui/QLayout>


#include "user_list.h"
#include "client.h"
#include "client_manager.h"
#include "main_window.h"



userList::userList( mainWindow * _main_window, QWidget * _parent ) :
	sideBarWidget( QPixmap( ":/resources/users.png" ),
			tr( "Logged in users" ),
			tr( "Open this workspace if you want to see which "
				"users currently are logged in." ),
						_main_window, _parent )
{
	m_list = new QListWidget( contentParent() );
	contentParent()->layout()->addWidget( m_list );
	m_list->setContextMenuPolicy( Qt::CustomContextMenu );
	m_list->setWhatsThis( tr( "Here you see the real names and the "
					"user-names of the users, logged in at "
					"currently visible clients." ) );
/*	connect( m_list, SIGNAL( customContextMenuRequested( const QPoint & ) ),
			this, SLOT( contextMenuRequested( const QPoint & ) ) );
	//m_list->setSelectionMode( Q3ListBox::Single );
	connect( m_list, SIGNAL( contextMenuRequested( Q3ListBoxItem *,
							const QPoint & ) ),
			this, SLOT( contextMenuHandler( Q3ListBoxItem *,
							const QPoint & ) ) );*/

	m_exportToFileBtn = new QPushButton(
					QPixmap( ":/resources/filesave.png" ),
						tr( "Export to text-file" ),
						contentParent() );
	contentParent()->layout()->addWidget( m_exportToFileBtn );
	connect( m_exportToFileBtn, SIGNAL( clicked() ),
					this, SLOT( clickedExportToFile() ) );
	m_exportToFileBtn->setWhatsThis( tr( "Use this button for exporting "
						"this user-list into a text-"
						"file. You can use this file "
						"later for collecting files "
						"after an exam has finished. "
						"This is sometimes neccessary, "
						"because some users might have "
						"finished and logged out "
						"earlier and so you cannot "
						"collect their files at the "
						"end of the exam." ) );
}




userList::~userList()
{
}




void userList::reload( void )
{
	QStringList users = userList::getLoggedInUsers(
					getMainWindow()->getClientManager() );

	QString selected_user;

	if( m_list->currentItem() != NULL )
	{
		selected_user = m_list->currentItem()->text();
	}

	m_list->clear();
	m_list->addItems( users );
	m_list->sortItems();

	QList<QListWidgetItem *> items = m_list->findItems( selected_user,
							Qt::MatchExactly );
	if( items.empty() == FALSE && selected_user != "" )
	{
		m_list->setItemSelected( items.front(), TRUE );
	}
}




client * userList::getClientFromUser( const QString & _user,
					clientManager * _client_manager )
{
	QVector<client *> clients = _client_manager->visibleClients();

	// loop through all clients
	for( QVector<client *>::iterator it = clients.begin();
						it != clients.end(); ++it )
	{
		QString cur_user = ( *it )->user();
		if( cur_user != "none" && !cur_user.isEmpty() )
		{
			return( *it );
		}
	}
	return( NULL );
}



/*
QString userList::getUserFromClient( const QString & _ip )
{
	QVector<client *> clients =
			getMainWindow()->getClientManager()->visibleClients();

	// loop through all clients
	for( QVector<client *>::iterator it = clients.begin();
						it != clients.end(); ++it )
	{
		if( ( *it )->localIP() == _ip )
		{
			return( ( *it )->user() );
		}
	}
	return( "" );
}*/




void userList::contextMenuHandler( const QPoint & _pos )
{
/*	if( m_list->itemAt( _pos ) != NULL )
	{
		client * c = userList::getClientFromUser( m_list->itemAt(
							_pos )->text(),
					getMainWindow()->getClientManager() );
		if( c != NULL )
		{
			QMenu context_menu( this );
			c->createActionMenu( &context_menu );
			context_menu.exec( QCursor::pos() );
		}
	}*/
}




void userList::clickedExportToFile( void )
{
	QString outfn = QFileDialog::getSaveFileName( this,
						tr( "Select output-file" ),
						QDir::homePath(),
						tr( "Text files (*.txt)" ) );
	if( outfn == "" )
	{
		return;
	}

	QStringList users = userList::getLoggedInUsers(
					getMainWindow()->getClientManager() );
	QString output = QDateTime::currentDateTime().toString() + "\n";

	for( QStringList::Iterator it = users.begin(); it != users.end(); ++it )
	{
		output += *it + "\n";
	}

	QFile outfile( outfn );
	outfile.open( QFile::WriteOnly );
	outfile.write( output.toAscii() );
	outfile.close();
}




QStringList userList::getLoggedInUsers( clientManager * _client_manager )
{
	QVector<client *> clients = _client_manager->visibleClients();

	QStringList users;

	// loop through all clients
	for( QVector<client *>::iterator it = clients.begin();
						it != clients.end(); ++it )
	{
		const QString user = ( *it )->user();
		if( user != "none" && !user.isEmpty() )
		{
			users.push_back( user );
		}
	}
	return( users );
}



#include "user_list.moc"

