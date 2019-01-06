/*
 * ComputerManagementView.cpp - provides a view for a network object tree
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

#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>

#include "ComputerManagementView.h"
#include "ComputerManager.h"
#include "NetworkObjectModel.h"
#include "RecursiveFilterProxyModel.h"
#include "RoomSelectionDialog.h"
#include "VeyonConfiguration.h"

#include "ui_ComputerManagementView.h"


ComputerManagementView::ComputerManagementView( ComputerManager& computerManager, QWidget *parent ) :
	QWidget(parent),
	ui(new Ui::ComputerManagementView),
	m_computerManager( computerManager ),
	m_filterProxyModel( new RecursiveFilterProxyModel( this ) )
{
	m_filterProxyModel->setSourceModel( computerManager.computerTreeModel() );
	m_filterProxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
	m_filterProxyModel->setFilterKeyColumn( -1 ); // filter all columns instead of first one only

	ui->setupUi(this);

	// capture keyboard events for tree view
	ui->treeView->installEventFilter( this );

	// set computer tree model as data model
	ui->treeView->setModel( m_filterProxyModel );

	// set default sort order
	ui->treeView->sortByColumn( 0, Qt::AscendingOrder );

	ui->addRoomButton->setVisible( VeyonCore::config().onlyCurrentRoomVisible() &&
										VeyonCore::config().manualRoomAdditionAllowed() );

	ui->filterLineEdit->setHidden( VeyonCore::config().computerFilterHidden() );

	connect( ui->filterLineEdit, &QLineEdit::textChanged,
			 this, &ComputerManagementView::updateFilter );
}



ComputerManagementView::~ComputerManagementView()
{
	delete ui;
}



bool ComputerManagementView::eventFilter( QObject *watched, QEvent *event )
{
	if( watched == ui->treeView &&
			event->type() == QEvent::KeyPress &&
			static_cast<QKeyEvent*>(event)->key() == Qt::Key_Delete )
	{
		removeRoom();
		return true;
	}

	return QWidget::eventFilter( watched, event );
}



void ComputerManagementView::addRoom()
{
	RoomSelectionDialog dialog( m_computerManager.networkObjectModel(), this );
	if( dialog.exec() && dialog.selectedRoom().isEmpty() == false )
	{
		m_computerManager.addRoom( dialog.selectedRoom() );
	}
}



void ComputerManagementView::removeRoom()
{
	auto model = m_computerManager.computerTreeModel();
	const auto index = ui->treeView->selectionModel()->currentIndex();

#if QT_VERSION < 0x050600
#warning Building legacy compat code for unsupported version of Qt
	if( index.isValid() &&
			static_cast<NetworkObject::Type>( model->data( index, NetworkObjectModel::TypeRole ).toInt() ) == NetworkObject::Group )
#else
	if( index.isValid() &&
			model->data( index, NetworkObjectModel::TypeRole ).value<NetworkObject::Type>() == NetworkObject::Group )
#endif
	{
		m_computerManager.removeRoom( model->data( index, NetworkObjectModel::NameRole ).toString() );
	}
}



void ComputerManagementView::saveList()
{
	QString fileName = QFileDialog::getSaveFileName( this, tr( "Select output filename" ),
													 QDir::homePath(), tr( "CSV files (*.csv)" ) );
	if( fileName.isEmpty() == false )
	{
		if( m_computerManager.saveComputerAndUsersList( fileName ) == false )
		{
			QMessageBox::critical( this, tr( "File error"),
								   tr( "Could not write the computer and users list to %1! "
									   "Please check the file access permissions." ).arg( fileName ) );
		}
	}
}



void ComputerManagementView::updateFilter()
{
	const auto filter = ui->filterLineEdit->text();
	auto model = ui->treeView->model();

	if( filter.isEmpty() )
	{
		m_filterProxyModel->setFilterWildcard( filter );

		for( int i = 0; i < model->rowCount(); ++i )
		{
			const auto index = model->index( i, 0 );
			ui->treeView->setExpanded( index, m_expandedGroups.contains( index ) );
		}

		m_previousFilter.clear();
	}
	else
	{
		if( m_previousFilter.isEmpty() )
		{
			m_expandedGroups.clear();

			for( int i = 0; i < model->rowCount(); ++i )
			{
				const auto index = model->index( i, 0 );
				if( ui->treeView->isExpanded( index ) )
				{
					m_expandedGroups.append( index );
				}
			}
		}

		m_previousFilter = filter;

		m_filterProxyModel->setFilterWildcard( filter );
		ui->treeView->expandAll();
	}
}
