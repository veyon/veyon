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

#include "ComputerManagementView.h"
#include "ComputerManager.h"

#include "ui_ComputerManagementView.h"


ComputerManagementView::ComputerManagementView( ComputerManager& computerManager, QWidget *parent ) :
	QWidget(parent),
	ui(new Ui::ComputerManagementView),
	m_computerManager( computerManager )
{
	ui->setupUi(this);

	ui->treeView->setModel( m_computerManager.networkObjectModel() );
	ui->treeView->sortByColumn( 0, Qt::AscendingOrder );
}



ComputerManagementView::~ComputerManagementView()
{
	delete ui;
}
