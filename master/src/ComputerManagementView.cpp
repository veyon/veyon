/*
 * ComputerManagementView.cpp - provides a view for a network object tree
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QFileDialog>
#include <QMessageBox>

#include "ComputerManagementView.h"
#include "ComputerManager.h"
#include "ItalcConfiguration.h"
#include "RoomSelectionDialog.h"

#include "ui_ComputerManagementView.h"


ComputerManagementView::ComputerManagementView( ComputerManager& computerManager, QWidget *parent ) :
	QWidget(parent),
	ui(new Ui::ComputerManagementView),
	m_computerManager( computerManager )
{
	ui->setupUi(this);

	ui->treeView->setModel( computerManager.computerTreeModel() );
	ui->treeView->sortByColumn( 0, Qt::AscendingOrder );

	ui->addRoomButton->setVisible( ItalcCore::config().onlyCurrentRoomVisible() &&
										ItalcCore::config().manualRoomAdditionAllowed() );
}



ComputerManagementView::~ComputerManagementView()
{
	delete ui;
}



void ComputerManagementView::addRoom()
{
	RoomSelectionDialog dialog( m_computerManager.networkObjectModel(), this );
	if( dialog.exec() && dialog.selectedRoom().isEmpty() == false )
	{
		m_computerManager.addRoom( dialog.selectedRoom() );
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
