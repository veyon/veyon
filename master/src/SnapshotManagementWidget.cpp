/*
 * SnapshotList.cpp - implementation of snapshot-list for side-bar
 *
 * Copyright (c) 2004-2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include <QFileSystemModel>
#include <QScrollArea>

#include "SnapshotManagementWidget.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"
#include "LocalSystem.h"
#include "Snapshot.h"

#include "ui_SnapshotManagementWidget.h"


SnapshotManagementWidget::SnapshotManagementWidget( QWidget *parent ) :
	QWidget( parent ),
	ui( new Ui::SnapshotManagementWidget ),
	m_fsModel( this )
{
	ui->setupUi( this );

	LocalSystem::Path::ensurePathExists( ItalcCore::config().snapshotDirectory() );

	m_fsModel.setNameFilters( QStringList() << "*.png" );
	m_fsModel.setFilter( QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files );
	m_fsModel.setRootPath( LocalSystem::Path::expand(
									ItalcCore::config().snapshotDirectory() ) );

	ui->list->setModel( &m_fsModel );
	ui->list->setRootIndex( m_fsModel.index( m_fsModel.rootPath() ) );

	connect( ui->list, &QListView::clicked, this, &SnapshotManagementWidget::snapshotSelected );
	connect( ui->list, &QListView::doubleClicked, this, &SnapshotManagementWidget::showSnapshot );

	connect( ui->showBtn, &QPushButton::clicked, this, &SnapshotManagementWidget::showSnapshot );
	connect( ui->deleteBtn, &QPushButton::clicked, this, &SnapshotManagementWidget::deleteSnapshot );
}




SnapshotManagementWidget::~SnapshotManagementWidget()
{
	delete ui;
}




void SnapshotManagementWidget::snapshotSelected( const QModelIndex &idx )
{
	Snapshot s( m_fsModel.filePath( idx ) );

	ui->previewLbl->setPixmap( s.pixmap() );
	ui->previewLbl->setMaximumWidth( window()->width()/2 );
	ui->previewLbl->setMaximumHeight( window()->height()/2 );

	ui->userLbl->setText( s.user() );
	ui->hostLbl->setText( s.host() );
	ui->dateLbl->setText( s.date() );
	ui->timeLbl->setText( s.time() );
}




void SnapshotManagementWidget::snapshotDoubleClicked( const QModelIndex &idx )
{
	QLabel * imgLabel = new QLabel;
	imgLabel->setPixmap( m_fsModel.filePath( idx ) );
	if( imgLabel->pixmap() != NULL )
	{
		imgLabel->setFixedSize( imgLabel->pixmap()->width(),
								imgLabel->pixmap()->height() );
	}

	QScrollArea * sa = new QScrollArea;
	sa->setAttribute( Qt::WA_DeleteOnClose, true );
	sa->move( 0, 0 );
	sa->setWidget( imgLabel );
	sa->setWindowTitle( m_fsModel.fileName( idx ) );
	sa->show();
}




void SnapshotManagementWidget::showSnapshot()
{
	if( ui->list->currentIndex().isValid() )
	{
		snapshotDoubleClicked( ui->list->currentIndex() );
	}
}




void SnapshotManagementWidget::deleteSnapshot()
{
	if( ui->list->currentIndex().isValid() )
	{
		m_fsModel.remove( ui->list->currentIndex() );
	}
}
