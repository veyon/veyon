/*
 * SpotlightPanel.cpp - implementation of SpotlightPanel
 *
 * Copyright (c) 2020 Tobias Junghans <tobydox@veyon.io>
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

#include <QGuiApplication>
#include <QListView>
#include <QMessageBox>

#include "ComputerSortFilterProxyModel.h"
#include "ComputerMonitoringWidget.h"
#include "SpotlightModel.h"
#include "SpotlightPanel.h"

#include "ui_SpotlightPanel.h"


SpotlightPanel::SpotlightPanel( ComputerMonitoringWidget* computerMonitoringWidget, QWidget* parent ) :
	QWidget( parent ),
	ui( new Ui::SpotlightPanel ),
	m_computerMonitoringWidget( computerMonitoringWidget ),
	m_model( new SpotlightModel( &m_computerMonitoringWidget->dataModel(), this ) )
{
	ui->setupUi( this );

	ui->monitoringWidget->setIgnoreWheelEvent( true );
	ui->monitoringWidget->setUseCustomComputerPositions( false );
	ui->monitoringWidget->listView()->setAcceptDrops( false );
	ui->monitoringWidget->listView()->setDragEnabled( false );
	ui->monitoringWidget->listView()->setModel( m_model );

	connect( ui->addButton, &QAbstractButton::clicked, this, &SpotlightPanel::add );
	connect( ui->removeButton, &QAbstractButton::clicked, this, &SpotlightPanel::remove );

	connect( m_computerMonitoringWidget->listView(), &QAbstractItemView::pressed, this, &SpotlightPanel::addPressedItem );
	connect( ui->monitoringWidget->listView(), &QAbstractItemView::pressed, this, &SpotlightPanel::removePressedItem );
}



SpotlightPanel::~SpotlightPanel()
{
	delete ui;
}



void SpotlightPanel::resizeEvent( QResizeEvent* event )
{
	const auto w = ui->monitoringWidget->listView()->width() - 40;
	const auto h = ui->monitoringWidget->listView()->height() - 40;

	ui->monitoringWidget->listView()->setIconSize( { qMin(w, h * 16 / 9),
							 qMin(h, w * 9 / 16) } );
	m_model->setIconSize( ui->monitoringWidget->listView()->iconSize() );

	QWidget::resizeEvent( event );
}



void SpotlightPanel::add()
{
	const auto selectedComputerControlInterfaces = m_computerMonitoringWidget->selectedComputerControlInterfaces();

	if( selectedComputerControlInterfaces.isEmpty() )
	{
		QMessageBox::information( this, tr("Spotlight"),
								  tr( "Please select at least one computer to add.") );
		return;
	}

	for( const auto& controlInterface : selectedComputerControlInterfaces )
	{
		m_model->add( controlInterface );
	}
}



void SpotlightPanel::remove()
{
	const auto selection = ui->monitoringWidget->listView()->selectionModel()->selectedIndexes();
	if( selection.isEmpty() )
	{
		QMessageBox::information( this, tr("Spotlight"),
								  tr( "Please select at least one computer to remove.") );
		return;
	}

	for( const auto& index : selection )
	{
		m_model->remove( m_model->data( index, SpotlightModel::ControlInterfaceRole )
							 .value<ComputerControlInterface::Pointer>() );
	}
}



void SpotlightPanel::addPressedItem( const QModelIndex& index )
{
	if( QGuiApplication::mouseButtons().testFlag( Qt::MidButton ) )
	{
		m_computerMonitoringWidget->listView()->selectionModel()->select( index, QItemSelectionModel::SelectCurrent );

		add();
	}
}



void SpotlightPanel::removePressedItem( const QModelIndex& index )
{
	if( QGuiApplication::mouseButtons().testFlag( Qt::MidButton ) )
	{
		ui->monitoringWidget->listView()->selectionModel()->select( index, QItemSelectionModel::SelectCurrent );

		remove();
	}
}
