/*
 * snapshot_list.cpp - implementation of snapshot-list for side-bar
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


class QColorGroup;

#include <QtCore/QDir>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QLayout>
#include <QtGui/QListWidget>


#include "snapshot_list.h"
#include "client.h"
#include "client_manager.h"
#include "local_system.h"



snapshotList::snapshotList( mainWindow * _main_window, QWidget * _parent ) :
	sideBarWidget( QPixmap( ":/resources/snapshot.png" ),
			tr( "Snapshots" ),
			tr( "Simply manage the snapshots you made using this "
				"workspace." ),
			_main_window, _parent )
{
	QFont f;
	f.setPixelSize( 12 );

	m_list = new QListWidget( contentParent() );
	contentParent()->layout()->addWidget( m_list );
	m_list->setFont( f );
	m_list->setSelectionMode( QListWidget::SingleSelection );
	m_list->setWhatsThis( tr( "All snapshots you made are listed here. "
				"You can make snapshots by clicking the "
				"camera-button in a client-window. "
				"These snapshots can be managed using the "
				"buttons below." ) );
	connect( m_list, SIGNAL( highlighted( const QString & ) ), this,
				SLOT( snapshotSelected( const QString & ) ) );
	connect( m_list, SIGNAL( selected( const QString & ) ), this,
			SLOT( snapshotDoubleClicked( const QString & ) ) );

	m_preview = new QLabel( contentParent() );
	contentParent()->layout()->addWidget( m_preview );
	m_preview->setScaledContents( TRUE );

	QWidget * hbox = new QWidget( contentParent() );
	contentParent()->layout()->addWidget( hbox );
	QHBoxLayout * hbox_layout = new QHBoxLayout( hbox );
	QWidget * vbox1 = new QWidget( hbox );
	QVBoxLayout * vbox1_layout = new QVBoxLayout( vbox1 );
	QWidget * vbox2 = new QWidget( hbox );
	QVBoxLayout * vbox2_layout = new QVBoxLayout( vbox2 );

	hbox_layout->addWidget( vbox1 );
	hbox_layout->addWidget( vbox2 );

	f.setBold( TRUE );
	f.setItalic( TRUE );

	QLabel * user_lbl = new QLabel( vbox1 );
	QLabel * date_lbl = new QLabel( vbox1 );
	QLabel * time_lbl = new QLabel( vbox1 );
	QLabel * client_lbl = new QLabel( vbox1 );
	vbox1_layout->addWidget( user_lbl );
	vbox1_layout->addWidget( date_lbl );
	vbox1_layout->addWidget( time_lbl );
	vbox1_layout->addWidget( client_lbl );
	user_lbl->setText( tr( "User:" ) );
	date_lbl->setText( tr( "Date:" ) );
	time_lbl->setText( tr( "Time:" ) );
	client_lbl->setText( tr( "Client:" ) );
	user_lbl->setFont( f );
	date_lbl->setFont( f );
	time_lbl->setFont( f );
	client_lbl->setFont( f );

	f.setBold( FALSE );
	f.setItalic( FALSE );

	m_user = new QLabel( vbox2 );
	m_date = new QLabel( vbox2 );
	m_time = new QLabel( vbox2 );
	m_client = new QLabel( vbox2 );
	vbox2_layout->addWidget( m_user );
	vbox2_layout->addWidget( m_date );
	vbox2_layout->addWidget( m_time );
	vbox2_layout->addWidget( m_client );
	m_user->setFont( f );
	m_date->setFont( f );
	m_time->setFont( f );
	m_client->setFont( f );

	vbox1->setFixedWidth( 72 );

	m_showBtn = new QPushButton( QPixmap( ":/resources/client_show.png" ),
							tr( "Show snapshot" ),
							contentParent() );
	contentParent()->layout()->addWidget( m_showBtn );
	connect( m_showBtn, SIGNAL( clicked() ), this,
						SLOT( showSnapshot() ) );
	m_showBtn->setWhatsThis( tr( "When clicking this button, the "
					"selected snapshot is being "
					"displayed in full size." ) );

	m_deleteBtn = new QPushButton( QPixmap( ":/resources/cancel.png" ),
				tr( "Delete snapshot" ), contentParent() );
	contentParent()->layout()->addWidget( m_deleteBtn );
	connect( m_deleteBtn, SIGNAL( clicked() ), this,
						SLOT( deleteSnapshot() ) );
	m_deleteBtn->setWhatsThis( tr( "When clicking on this button, the "
				"selected snapshot is being deleted." ) );

	m_reloadBtn = new QPushButton( QPixmap( ":/resources/reload.png" ),
					tr( "Reload list" ), contentParent() );
	contentParent()->layout()->addWidget( m_reloadBtn );
	connect( m_reloadBtn, SIGNAL( clicked() ), this, SLOT( reloadList() ) );
	m_reloadBtn->setWhatsThis( tr( "When clicking on this button, the "
					"snapshot-list is being reloaded "
					"immediately. You actually should "
					"never need this function since iTALC "
					"automatically reloads this list, "
					"after making a new snapshot." ) );

	reloadList();
}




snapshotList::~snapshotList()
{
}





void snapshotList::snapshotSelected( const QString & _s )
{
	m_preview->setPixmap( localSystem::snapshotDir() + _s );
	m_preview->setFixedHeight( m_preview->width() * 3 / 4 );
	m_user->setText( _s.section( '_', 0, 0 ) );
 	m_client->setText( _s.section( '_', 1, 1 ) );
	m_date->setText( QDate::fromString( _s.section( '_', 2, 2 ),
				Qt::ISODate ).toString( Qt::LocalDate ) );
	m_time->setText( _s.section( '_', 3, 3 ).section( '.', 0, 0 ) );
}




void snapshotList::snapshotDoubleClicked( const QString & _s )
{
	// maybe the user clicked on "show snapshot" and selected no
	// snapshot...
	if( _s == "" )
	{
		return;
	}

	QWidget * w = new QWidget;
	w->setAttribute( Qt::WA_DeleteOnClose, TRUE );
	w->move( 0, 0 );
	w->setWindowTitle( _s );

	QLabel * img_label = new QLabel( w );
	img_label->setPixmap( localSystem::snapshotDir() + _s );
	if( img_label->pixmap() != NULL )
	{
		w->setFixedSize( img_label->pixmap()->width(),
					img_label->pixmap()->height() );
		img_label->setFixedSize( img_label->pixmap()->width(),
					img_label->pixmap()->height() );
	}

	w->show();
}




void snapshotList::showSnapshot( void )
{
	if( m_list->currentItem() )
	{
		snapshotDoubleClicked( m_list->currentItem()->text() );
	}
}




void snapshotList::deleteSnapshot( void )
{
	if( !m_list->currentItem() )
	{
		return;
	}

	const QString s = m_list->currentItem()->text();

	// maybe the user clicked on "delete snapshot" and didn't select a
	// snapshot...
	if( s.isEmpty() )
	{
		return;
	}

	QFile( localSystem::snapshotDir() + s ).remove();

	reloadList();
}




void snapshotList::reloadList( void )
{
	QDir sdir( localSystem::snapshotDir(), "*.png",
						QDir::Name | QDir::IgnoreCase,
						QDir::Files | QDir::Readable );

	m_list->clear();
	m_list->insertItems( 0, sdir.entryList() );
}



#include "snapshot_list.moc"

