/*
 * SlideshowPanel.cpp - implementation of SlideshowPanel
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

#include <QListView>

#include "ComputerSortFilterProxyModel.h"
#include "ComputerMonitoringWidget.h"
#include "SlideshowModel.h"
#include "SlideshowPanel.h"
#include "UserConfig.h"

#include "ui_SlideshowPanel.h"


SlideshowPanel::SlideshowPanel( UserConfig& config, ComputerMonitoringWidget* computerMonitoringWidget, QWidget *parent ) :
	QWidget( parent ),
	ui( new Ui::SlideshowPanel ),
	m_config( config ),
	m_model( new SlideshowModel( &computerMonitoringWidget->dataModel(), this ) )
{
	ui->setupUi( this );

	ui->monitoringWidget->setUseCustomComputerPositions( false );
	ui->monitoringWidget->setIgnoreWheelEvent( true );
	ui->monitoringWidget->listView()->setAcceptDrops( false );
	ui->monitoringWidget->listView()->setDragEnabled( false );
	ui->monitoringWidget->listView()->setSelectionMode( QListView::SingleSelection );
	ui->monitoringWidget->listView()->setModel( m_model );

	connect( ui->monitoringWidget->listView(), &QListView::iconSizeChanged, m_model, &SlideshowModel::setIconSize );

	connect( ui->startStopButton, &QAbstractButton::toggled, this, &SlideshowPanel::updateDuration );
	connect( ui->durationSlider, &QSlider::valueChanged, this, &SlideshowPanel::updateDuration );

	connect( ui->showPreviousButton, &QAbstractButton::clicked, m_model, &SlideshowModel::showPrevious );
	connect( ui->showNextButton, &QAbstractButton::clicked, m_model, &SlideshowModel::showNext );

	ui->durationSlider->setValue( m_config.slideshowDuration() );

	updateDuration();
}



SlideshowPanel::~SlideshowPanel()
{
	delete ui;
}



void SlideshowPanel::resizeEvent( QResizeEvent* event )
{
	const auto w = ui->monitoringWidget->listView()->width() - 40;
	const auto h = ui->monitoringWidget->listView()->height() - 40;

	ui->monitoringWidget->listView()->setIconSize( { qMin(w, h * 16 / 9),
							 qMin(h, w * 9 / 16) } );

	QWidget::resizeEvent( event );
}



void SlideshowPanel::updateDuration()
{
	const int duration = ui->durationSlider->value();

	m_model->setRunning( ui->startStopButton->isChecked(), duration );
	m_config.setSlideshowDuration( duration );

	ui->durationLabel->setText( QStringLiteral("%1 s").arg( duration / 1000 ) );
}
