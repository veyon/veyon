/*
 * RoomSelectionDialog.cpp - header file for RoomSelectionDialog
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "RoomSelectionDialog.h"

#include "ui_RoomSelectionDialog.h"

RoomSelectionDialog::RoomSelectionDialog( QAbstractItemModel* roomListModel, QWidget* parent ) :
	QDialog( parent ),
	ui( new Ui::RoomSelectionDialog ),
	m_sortFilterProxyModel( this )
{
	ui->setupUi( this );

	m_sortFilterProxyModel.setSourceModel( roomListModel );
	m_sortFilterProxyModel.sort( 0 );

	ui->listView->setModel( &m_sortFilterProxyModel );

	connect( ui->listView->selectionModel(), &QItemSelectionModel::currentChanged,
			 this, &RoomSelectionDialog::updateSelection );

	updateSearchFilter();
}



RoomSelectionDialog::~RoomSelectionDialog()
{
	delete ui;
}



void RoomSelectionDialog::updateSearchFilter()
{
	m_sortFilterProxyModel.setFilterRegExp( QRegExp( ui->filterLineEdit->text() ) );
	m_sortFilterProxyModel.setFilterCaseSensitivity( Qt::CaseInsensitive );

	ui->listView->selectionModel()->setCurrentIndex( m_sortFilterProxyModel.index( 0, 0 ),
														 QItemSelectionModel::ClearAndSelect );
}



void RoomSelectionDialog::updateSelection( const QModelIndex& current, const QModelIndex& previous )
{
	m_selectedRoom = m_sortFilterProxyModel.data( current ).toString();
}
