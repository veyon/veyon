/*
 * ComputerManagementView.cpp - provides a view for a network object tree
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "VeyonConfiguration.h"
#include "RoomSelectionDialog.h"

#include "ui_ComputerManagementView.h"


ComputerManagementView::ComputerManagementView( ComputerManager& computerManager, QWidget *parent ) :
	QWidget(parent),
	ui(new Ui::ComputerManagementView),
	m_computerManager( computerManager )
{
	ui->setupUi(this);

	// capture keyboard events for tree view
	ui->treeView->installEventFilter( this );

	// set computer tree model as data model
	ui->treeView->setModel( computerManager.computerTreeModel() );

	// set default sort order
	ui->treeView->sortByColumn( 0, Qt::AscendingOrder );

	ui->addRoomButton->setVisible( VeyonCore::config().onlyCurrentRoomVisible() &&
										VeyonCore::config().manualRoomAdditionAllowed() );
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

	if( index.isValid() &&
			model->data( index, NetworkObjectModel::TypeRole ).value<NetworkObject::Type>() == NetworkObject::Group )
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
