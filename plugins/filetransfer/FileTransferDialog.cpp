/*
 * FileTransferDialog.cpp - implementation of FileTransferDialog
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QPushButton>

#include "FileTransferController.h"
#include "FileTransferDialog.h"
#include "FileTransferListModel.h"

#include "ui_FileTransferDialog.h"

FileTransferDialog::FileTransferDialog( FileTransferController* controller, QWidget* parent ) :
	QDialog( parent ),
	ui( new Ui::FileTransferDialog ),
	m_controller( controller ),
	m_listModel( new FileTransferListModel( m_controller, this ) )
{
	ui->setupUi( this );
	ui->buttonBox->button( QDialogButtonBox::Ok )->setText( tr( "Start" ) );

	ui->fileListView->setModel( m_listModel );

	connect( m_controller, &FileTransferController::progressChanged,
			 this, &FileTransferDialog::updateProgress );

	connect( m_controller, &FileTransferController::finished,
			 this, &FileTransferDialog::finish );
}



FileTransferDialog::~FileTransferDialog()
{
	delete ui;

	delete m_listModel;
}



void FileTransferDialog::accept()
{
	ui->optionsGroupBox->setDisabled( true );
	ui->buttonBox->setStandardButtons( QDialogButtonBox::Cancel );

	FileTransferController::Flags flags( FileTransferController::Transfer );

	if( ui->transferAndOpenFolder->isChecked() )
	{
		flags |= FileTransferController::OpenTransferFolder;
	}

	if( ui->transferAndOpenProgram->isChecked() )
	{
		flags |= FileTransferController::OpenFilesInApplication;
	}

	if( ui->overwriteExistingFiles->isChecked() )
	{
		flags |= FileTransferController::OverwriteExistingFiles;
	}

	m_controller->setFlags( flags );
	m_controller->start();
}



void FileTransferDialog::reject()
{
	if( m_controller->isRunning() )
	{
		m_controller->stop();
	}

	QDialog::reject();
}



void FileTransferDialog::finish()
{
	ui->buttonBox->setStandardButtons( QDialogButtonBox::Close );
	connect( ui->buttonBox->button( QDialogButtonBox::Close ), &QPushButton::clicked,
			 this, &FileTransferDialog::close );
}



void FileTransferDialog::updateProgress( int progress )
{
	ui->progressBar->setValue( progress );
}
