/*
 * LocationDialog.cpp - header file for LocationDialog
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "LocationDialog.h"

#include "ui_LocationDialog.h"

LocationDialog::LocationDialog( QAbstractItemModel* locationListModel, QWidget* parent ) :
	QDialog( parent ),
	ui( new Ui::LocationDialog ),
	m_sortFilterProxyModel( this )
{
	ui->setupUi( this );

	m_sortFilterProxyModel.setSourceModel( locationListModel );
	m_sortFilterProxyModel.sort( 0 );

	ui->listView->setModel( &m_sortFilterProxyModel );

	connect( ui->listView->selectionModel(), &QItemSelectionModel::currentChanged,
			 this, &LocationDialog::updateSelection );

	updateSearchFilter();
}



LocationDialog::~LocationDialog()
{
	delete ui;
}



void LocationDialog::updateSearchFilter()
{
	m_sortFilterProxyModel.setFilterRegExp( QRegExp( ui->filterLineEdit->text() ) );
	m_sortFilterProxyModel.setFilterCaseSensitivity( Qt::CaseInsensitive );

	ui->listView->selectionModel()->setCurrentIndex( m_sortFilterProxyModel.index( 0, 0 ),
														 QItemSelectionModel::ClearAndSelect );
}



void LocationDialog::updateSelection( const QModelIndex& current, const QModelIndex& previous )
{
	Q_UNUSED(previous)

	m_selectedLocation = m_sortFilterProxyModel.data( current ).toString();
}
