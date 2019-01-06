/*
 * FileTransferListModel.cpp - implementation of FileTransferListModel class
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

#include "FileTransferListModel.h"
#include "FileTransferController.h"


FileTransferListModel::FileTransferListModel( FileTransferController* controller, QObject* parent ) :
	QStringListModel( parent ),
	m_controller( controller ),
	m_scheduledPixmap( QIcon( QSL( ":/filetransfer/file-scheduled.png" ) ) ),
	m_transferringPixmap( QIcon( QSL( ":/filetransfer/file-transferring.png" ) ) ),
	m_finishedPixmap( QIcon( QSL( ":/filetransfer/file-finished.png" ) ) )
{
	setStringList( m_controller->files() );

	connect( m_controller, &FileTransferController::filesChanged,
			 this, [this]() { setStringList( m_controller->files() ); } );

	connect( m_controller, &FileTransferController::progressChanged,
			 this, [this]() { emit dataChanged( index( 0 ), index( rowCount() ), { Qt::DecorationRole } ); } );

	connect( m_controller, &FileTransferController::started,
			 this, [this]() { setStringList( m_controller->files() ); } );
}



QVariant FileTransferListModel::data( const QModelIndex& index, int role ) const
{
	if( index.isValid() && role == Qt::DecorationRole )
	{
		int currentRow = m_controller->currentFileIndex();

		if( index.row() < currentRow )
		{
			return m_finishedPixmap;
		}
		else if( index.row() > currentRow || m_controller->isRunning() == false )
		{
			return m_scheduledPixmap;
		}

		return m_transferringPixmap;
	}

	return QStringListModel::data( index, role );
}
