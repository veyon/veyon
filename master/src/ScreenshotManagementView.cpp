/*
 * ScreenshotManagementView.cpp - implementation of screenshot management view
 *
 * Copyright (c) 2004-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QDir>
#include <QDate>
#include <QFileSystemModel>
#include <QScrollArea>

#include "Filesystem.h"
#include "ScreenshotManagementView.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "Screenshot.h"

#include "ui_ScreenshotManagementView.h"


ScreenshotManagementView::ScreenshotManagementView( QWidget *parent ) :
	QWidget( parent ),
	ui( new Ui::ScreenshotManagementView ),
	m_fsModel( this )
{
	ui->setupUi( this );

	VeyonCore::filesystem().ensurePathExists( VeyonCore::config().screenshotDirectory() );

	m_fsModel.setNameFilters( { QStringLiteral("*.png") } );
	m_fsModel.setFilter( QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files );
	m_fsModel.setRootPath( VeyonCore::filesystem().expandPath( VeyonCore::config().screenshotDirectory() ) );

	ui->list->setModel( &m_fsModel );
	ui->list->setRootIndex( m_fsModel.index( m_fsModel.rootPath() ) );

	connect( ui->list, &QListView::clicked, this, &ScreenshotManagementView::screenshotSelected );
	connect( ui->list, &QListView::doubleClicked, this, &ScreenshotManagementView::showScreenshot );

	connect( ui->showBtn, &QPushButton::clicked, this, &ScreenshotManagementView::showScreenshot );
	connect( ui->deleteBtn, &QPushButton::clicked, this, &ScreenshotManagementView::deleteScreenshot );
}




ScreenshotManagementView::~ScreenshotManagementView()
{
	delete ui;
}



void ScreenshotManagementView::resizeEvent( QResizeEvent* event )
{
	int maxWidth = contentsRect().width();
	int maxHeight = maxWidth * 9 / 16;

	ui->previewLbl->setMaximumSize( maxWidth, maxHeight );

	QWidget::resizeEvent( event );
}




void ScreenshotManagementView::screenshotSelected( const QModelIndex &idx )
{
	Screenshot s( m_fsModel.filePath( idx ) );

	ui->previewLbl->setPixmap( s.pixmap() );

	ui->userLbl->setText( s.user() );
	ui->hostLbl->setText( s.host() );
	ui->dateLbl->setText( s.date() );
	ui->timeLbl->setText( s.time() );
}




void ScreenshotManagementView::screenshotDoubleClicked( const QModelIndex &idx )
{
	auto imgLabel = new QLabel;
	imgLabel->setPixmap( m_fsModel.filePath( idx ) );
	if( imgLabel->pixmap() != nullptr )
	{
		imgLabel->setFixedSize( imgLabel->pixmap()->width(),
								imgLabel->pixmap()->height() );
	}

	auto sa = new QScrollArea;
	sa->setAttribute( Qt::WA_DeleteOnClose, true );
	sa->move( 0, 0 );
	sa->setWidget( imgLabel );
	sa->setWindowTitle( m_fsModel.fileName( idx ) );
	sa->show();
}




void ScreenshotManagementView::showScreenshot()
{
	if( ui->list->currentIndex().isValid() )
	{
		screenshotDoubleClicked( ui->list->currentIndex() );
	}
}




void ScreenshotManagementView::deleteScreenshot()
{
	if( ui->list->currentIndex().isValid() )
	{
		m_fsModel.remove( ui->list->currentIndex() );
	}
}
