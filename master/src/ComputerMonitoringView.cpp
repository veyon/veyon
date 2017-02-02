/*
 * ComputerMonitoringView.cpp - provides a view with computer monitor thumbnails
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

#include "ComputerManager.h"
#include "ComputerMonitoringView.h"
#include "MasterCore.h"

#include "ui_ComputerMonitoringView.h"

ComputerMonitoringView::ComputerMonitoringView( QWidget *parent ) :
	QWidget(parent),
	ui(new Ui::ComputerMonitoringView),
	m_computerManager( nullptr ),
	m_computerListModel( nullptr )
{
	ui->setupUi( this );

	connect( ui->gridSizeSlider, &QSlider::valueChanged,
			 this, &ComputerMonitoringView::setComputerScreenSize );
}



ComputerMonitoringView::~ComputerMonitoringView()
{
	delete ui;

	delete m_computerListModel;
}



void ComputerMonitoringView::setComputerManager( ComputerManager &computerManager )
{
	delete m_computerListModel;

	m_computerManager = &computerManager;

	m_computerListModel = new ComputerListModel( computerManager, this );

	ui->listView->setModel( m_computerListModel );
}



void ComputerMonitoringView::setComputerScreenSize(int size)
{
	if( m_computerManager )
	{
		m_computerManager->setComputerScreenSize( QSize( size, size * 9 / 16 ) );
		ui->listView->updateGeometry();
	}
}
