/*
 * snapshot_list.cpp - implementation of snapshot-list for side-bar
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QDir>
#include <QtCore/QDate>
#include <QtGui/QFileSystemModel>
#include <QtGui/QScrollArea>

#include "SnapshotList.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"
#include "LocalSystem.h"
#include "Snapshot.h"

#include "ui_Snapshots.h"


SnapshotList::SnapshotList( MainWindow *mainWindow, QWidget *parent ) :
	SideBarWidget( QPixmap( ":/resources/snapshot.png" ),
			tr( "Snapshots" ),
			tr( "Simply manage the snapshots you made using this workspace." ),
			mainWindow, parent ),
	ui( new Ui::Snapshots ),
	m_fsModel( new QFileSystemModel( this ) )
{
	ui->setupUi( contentParent() );

	LocalSystem::Path::ensurePathExists( ItalcCore::config->snapshotDirectory() );

	m_fsModel->setNameFilters( QStringList() << "*.png" );
	m_fsModel->setFilter( QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files );
	m_fsModel->setRootPath( LocalSystem::Path::expand(
									ItalcCore::config->snapshotDirectory() ) );

	ui->list->setModel( m_fsModel );
	ui->list->setRootIndex( m_fsModel->index( m_fsModel->rootPath() ) );

	connect( ui->list, SIGNAL( clicked( const QModelIndex & ) ),
				this, SLOT( snapshotSelected( const QModelIndex & ) ) );
	connect( ui->list, SIGNAL( doubleClicked( const QModelIndex & ) ),
				this, SLOT( showSnapshot() ) );

	connect( ui->showBtn, SIGNAL( clicked() ), this, SLOT( showSnapshot() ) );
	connect( ui->deleteBtn, SIGNAL( clicked() ), this, SLOT( deleteSnapshot() ) );
}




SnapshotList::~SnapshotList()
{
}




void SnapshotList::snapshotSelected( const QModelIndex &idx )
{
	Snapshot s( m_fsModel->filePath( idx ) );

	ui->previewLbl->setPixmap( s.pixmap() );
	ui->previewLbl->setFixedHeight( ui->previewLbl->width() * 3 / 4 );

	ui->userLbl->setText( s.user() );
	ui->hostLbl->setText( s.host() );
	ui->dateLbl->setText( s.date() );
	ui->timeLbl->setText( s.time() );
}




void SnapshotList::snapshotDoubleClicked( const QModelIndex &idx )
{
	QLabel * imgLabel = new QLabel;
	imgLabel->setPixmap( m_fsModel->filePath( idx ) );
	if( imgLabel->pixmap() != NULL )
	{
		imgLabel->setFixedSize( imgLabel->pixmap()->width(),
								imgLabel->pixmap()->height() );
	}

	QScrollArea * sa = new QScrollArea;
	sa->setAttribute( Qt::WA_DeleteOnClose, true );
	sa->move( 0, 0 );
	sa->setWidget( imgLabel );
	sa->setWindowTitle( m_fsModel->fileName( idx ) );
	sa->show();
}




void SnapshotList::showSnapshot()
{
	if( ui->list->currentIndex().isValid() )
	{
		snapshotDoubleClicked( ui->list->currentIndex() );
	}
}




void SnapshotList::deleteSnapshot()
{
	if( ui->list->currentIndex().isValid() )
	{
		m_fsModel->remove( ui->list->currentIndex() );
	}
}



