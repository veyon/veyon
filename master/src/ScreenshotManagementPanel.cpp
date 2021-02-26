/*
 * ScreenshotManagementPanel.cpp - implementation of screenshot management view
 *
 * Copyright (c) 2004-2021 Tobias Junghans <tobydox@veyon.io>
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
#include <QFileInfo>
#include <QMessageBox>

#include "Filesystem.h"
#include "ScreenshotManagementPanel.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "Screenshot.h"

#include "ui_ScreenshotManagementPanel.h"


ScreenshotManagementPanel::ScreenshotManagementPanel( QWidget *parent ) :
	QWidget( parent ),
	ui( new Ui::ScreenshotManagementPanel )
{
	ui->setupUi( this );

	VeyonCore::filesystem().ensurePathExists( VeyonCore::config().screenshotDirectory() );

	m_fsWatcher.addPath( VeyonCore::filesystem().screenshotDirectoryPath() );
	connect( &m_fsWatcher, &QFileSystemWatcher::directoryChanged, this, &ScreenshotManagementPanel::updateModel );

	m_reloadTimer.setInterval( FsModelResetDelay );
	m_reloadTimer.setSingleShot( true );

	connect( &m_reloadTimer, &QTimer::timeout, this, &ScreenshotManagementPanel::updateModel );
	connect( &VeyonCore::filesystem(), &Filesystem::screenshotDirectoryModified,
			 &m_reloadTimer, QOverload<>::of(&QTimer::start) );

	ui->list->setModel( &m_model );

	connect( ui->list->selectionModel(), &QItemSelectionModel::currentRowChanged,
			 this, &ScreenshotManagementPanel::updateScreenshot );
	connect( ui->list, &QListView::activated, this, &ScreenshotManagementPanel::showScreenshot );

	connect( ui->showBtn, &QPushButton::clicked, this, &ScreenshotManagementPanel::showScreenshot );
	connect( ui->deleteBtn, &QPushButton::clicked, this, &ScreenshotManagementPanel::deleteScreenshot );

	updateModel();
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



void ScreenshotManagementPanel::updateModel()
{
	const auto currentFile = m_model.data( ui->list->currentIndex(), Qt::DisplayRole ).toString();

	const QDir dir{ VeyonCore::filesystem().screenshotDirectoryPath() };
	const auto files = dir.entryList( { QStringLiteral("*.png") },
									  QDir::Filter::Files, QDir::SortFlag::Name );

	m_model.setStringList( files );

	ui->list->setCurrentIndex( m_model.index( files.indexOf( currentFile ) ) );
}



QString ScreenshotManagementPanel::filePath( const QModelIndex& index ) const
{
	return VeyonCore::filesystem().screenshotDirectoryPath() + QDir::separator() + m_model.data( index, Qt::DisplayRole ).toString();
}



void ScreenshotManagementPanel::updateScreenshot( const QModelIndex& index )
{
	setPreview( Screenshot( filePath( index ) ) );
}



void ScreenshotManagementPanel::screenshotDoubleClicked( const QModelIndex& index )
{
	auto screenshotWindow = new QLabel;
	screenshotWindow->setPixmap( filePath( index ) );
	screenshotWindow->setScaledContents( true );
	screenshotWindow->setWindowTitle( QFileInfo( filePath( index ) ).fileName() );
	screenshotWindow->setAttribute( Qt::WA_DeleteOnClose, true );
	screenshotWindow->showNormal();
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
	const auto selection = ui->list->selectionModel()->selectedIndexes();
	if( selection.size() > 1 &&
		QMessageBox::question( this,
							   tr("Screenshot"),
							   tr("Do you really want to delete all selected screenshots?") ) != QMessageBox::Yes )
	{
		return;
	}

	for( const auto& index : selection )
	{
		QFile::remove( filePath( index ) );
	}

	updateModel();
}
