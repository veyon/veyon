/*
 * ScreenshotManagementPanel.cpp - implementation of screenshot management view
 *
 * Copyright (c) 2004-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QDir>
#include <QDate>
#include <QFileSystemModel>
#include <QScrollArea>

#include "Filesystem.h"
#include "ScreenshotManagementPanel.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "Screenshot.h"

#include "ui_ScreenshotManagementPanel.h"


ScreenshotManagementPanel::ScreenshotManagementPanel( QWidget *parent ) :
	QWidget( parent ),
	ui( new Ui::ScreenshotManagementPanel ),
	m_fsModel( this )
{
	ui->setupUi( this );

	VeyonCore::filesystem().ensurePathExists( VeyonCore::config().screenshotDirectory() );

	m_fsModel.setNameFilters( { QStringLiteral("*.png") } );
	m_fsModel.setFilter( QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files );
	m_fsModel.setRootPath( VeyonCore::filesystem().expandPath( VeyonCore::config().screenshotDirectory() ) );

	ui->list->setModel( &m_fsModel );
	ui->list->setRootIndex( m_fsModel.index( m_fsModel.rootPath() ) );

	connect( ui->list, &QListView::clicked, this, &ScreenshotManagementPanel::updateScreenshot );
	connect( ui->list, &QListView::doubleClicked, this, &ScreenshotManagementPanel::showScreenshot );

	connect( ui->showBtn, &QPushButton::clicked, this, &ScreenshotManagementPanel::showScreenshot );
	connect( ui->deleteBtn, &QPushButton::clicked, this, &ScreenshotManagementPanel::deleteScreenshot );
}



ScreenshotManagementPanel::~ScreenshotManagementPanel()
{
	delete ui;
}



void ScreenshotManagementPanel::setPreview( const Screenshot& screenshot )
{
	ui->previewLbl->setPixmap( QPixmap::fromImage( screenshot.image() ) );

	ui->userLbl->setText( screenshot.user() );
	ui->hostLbl->setText( screenshot.host() );
	ui->dateLbl->setText( screenshot.date() );
	ui->timeLbl->setText( screenshot.time() );
}



void ScreenshotManagementPanel::resizeEvent( QResizeEvent* event )
{
	int maxWidth = contentsRect().width();
	int maxHeight = maxWidth * 9 / 16;

	ui->previewLbl->setMaximumSize( maxWidth, maxHeight );

	QWidget::resizeEvent( event );
}



void ScreenshotManagementPanel::updateScreenshot( const QModelIndex &idx )
{
	setPreview( Screenshot( m_fsModel.filePath( idx ) ) );
}



void ScreenshotManagementPanel::screenshotDoubleClicked( const QModelIndex &idx )
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



void ScreenshotManagementPanel::showScreenshot()
{
	if( ui->list->currentIndex().isValid() )
	{
		screenshotDoubleClicked( ui->list->currentIndex() );
	}
}



void ScreenshotManagementPanel::deleteScreenshot()
{
	if( ui->list->currentIndex().isValid() )
	{
		m_fsModel.remove( ui->list->currentIndex() );
	}
}
