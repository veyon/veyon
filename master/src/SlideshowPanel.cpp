/*
 * SlideshowPanel.cpp - implementation of SlideshowPanel
 *
 * Copyright (c) 2020-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "ComputerMonitoringModel.h"
#include "ComputerMonitoringWidget.h"
#include "SlideshowModel.h"
#include "SlideshowPanel.h"
#include "UserConfig.h"

#include "ui_SlideshowPanel.h"


SlideshowPanel::SlideshowPanel( UserConfig& config, ComputerMonitoringWidget* computerMonitoringWidget, QWidget *parent ) :
	QWidget( parent ),
	ui( new Ui::SlideshowPanel ),
	m_config( config ),
	m_model( new SlideshowModel( computerMonitoringWidget->dataModel(), this ) )
{
	ui->setupUi( this );

	ui->monitoringWidget->setAutoAdjustIconSize( false );
	ui->monitoringWidget->setUseCustomComputerPositions( false );
	ui->monitoringWidget->setAcceptDrops( false );
	ui->monitoringWidget->setDragEnabled( false );
	ui->monitoringWidget->setIgnoreWheelEvent( true );
	ui->monitoringWidget->setSelectionMode( QListView::SingleSelection );
	ui->monitoringWidget->setModel( m_model );

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
	static constexpr auto ExtraMargin = 10;

	const auto spacing = ui->monitoringWidget->spacing();
	const auto labelHeight = ui->monitoringWidget->fontMetrics().height();

	ui->monitoringWidget->setIconSize( { ui->monitoringWidget->width() - ExtraMargin - spacing * 2,
										 ui->monitoringWidget->height() - ExtraMargin - labelHeight - spacing * 2 } );

	m_model->setIconSize( ui->monitoringWidget->iconSize() );

	QWidget::resizeEvent( event );
}



void SlideshowPanel::updateDuration()
{
	const int duration = ui->durationSlider->value();

	m_model->setRunning( ui->startStopButton->isChecked(), duration );
	m_config.setSlideshowDuration( duration );

	ui->durationLabel->setText( QStringLiteral("%1 s").arg( duration / 1000 ) );
}
