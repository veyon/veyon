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
#include <QMessageBox>

#include "ComputerMonitoringModel.h"
#include "ComputerMonitoringWidget.h"
#include "SpotlightModel.h"
#include "SpotlightPanel.h"
#include "UserConfig.h"

#include "ui_SpotlightPanel.h"


SpotlightPanel::SpotlightPanel( UserConfig& config, ComputerMonitoringWidget* computerMonitoringWidget, QWidget* parent ) :
	QWidget( parent ),
	ui( new Ui::SpotlightPanel ),
	m_config( config ),
	m_computerMonitoringWidget( computerMonitoringWidget ),
	m_model( new SpotlightModel( m_computerMonitoringWidget->dataModel(), this ) )
{
	ui->setupUi( this );

	ui->list->setAutoAdjustIconSize( false );
	ui->list->setUseCustomComputerPositions( false );
	ui->list->setAcceptDrops( false );
	ui->list->setDragEnabled( false );
	ui->list->setIgnoreWheelEvent( true );
	ui->list->setModel( m_model );

	connect( ui->addButton, &QAbstractButton::clicked, this, &SpotlightPanel::add );
	connect( ui->removeButton, &QAbstractButton::clicked, this, &SpotlightPanel::remove );
	connect( ui->realtimeViewButton, &QAbstractButton::toggled, this, &SpotlightPanel::setRealtimeView );

	connect( m_computerMonitoringWidget, &QAbstractItemView::pressed, this, &SpotlightPanel::addPressedItem );
	connect( ui->list, &QAbstractItemView::pressed, this, &SpotlightPanel::removePressedItem );

	connect( m_model, &QAbstractItemModel::rowsRemoved, this, [=]() {
		if( m_model->rowCount() <= 0 )
		{
			ui->stackedWidget->setCurrentWidget( ui->helpPage );
		}
	} );

	setRealtimeView( m_config.spotlightRealtime() );
}



SpotlightPanel::~SpotlightPanel()
{
	delete ui;
}



void SpotlightPanel::resizeEvent( QResizeEvent* event )
{
	updateIconSize();

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

	if( ui->stackedWidget->currentWidget() != ui->viewPage )
	{
		ui->stackedWidget->setCurrentWidget( ui->viewPage );

		// due to a bug in QListView force relayout of all items to show decorations (thumbnails) properly
		updateIconSize();
	}
}



void SpotlightPanel::remove()
{
	const auto selection = ui->list->selectionModel()->selectedIndexes();
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



void SpotlightPanel::setRealtimeView( bool enabled )
{
	m_model->setUpdateInRealtime( enabled );

	m_config.setSpotlightRealtime( enabled );

	ui->realtimeViewButton->setChecked( enabled );
}



void SpotlightPanel::updateIconSize()
{
	static constexpr auto ExtraMargin = 10;

	const auto spacing = ui->list->spacing();
	const auto labelHeight = ui->list->fontMetrics().height();

	ui->list->setIconSize( { ui->list->width() - ExtraMargin - spacing * 2,
							 ui->list->height() - ExtraMargin - labelHeight - spacing * 2 } );
	m_model->setIconSize( ui->list->iconSize() );
}



void SpotlightPanel::addPressedItem( const QModelIndex& index )
{
	if( QGuiApplication::mouseButtons().testFlag( Qt::MidButton ) )
	{
		m_computerMonitoringWidget->selectionModel()->select( index, QItemSelectionModel::SelectCurrent );

		add();
	}
}



void SpotlightPanel::removePressedItem( const QModelIndex& index )
{
	if( QGuiApplication::mouseButtons().testFlag( Qt::MidButton ) )
	{
		ui->list->selectionModel()->select( index, QItemSelectionModel::SelectCurrent );

		remove();
	}
}
