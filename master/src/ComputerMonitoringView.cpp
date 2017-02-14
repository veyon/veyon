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

#include <QMenu>

#include "ComputerManager.h"
#include "ComputerMonitoringView.h"
#include "FeatureManager.h"
#include "PersonalConfig.h"

#include "ui_ComputerMonitoringView.h"

ComputerMonitoringView::ComputerMonitoringView( QWidget *parent ) :
	QWidget(parent),
	ui(new Ui::ComputerMonitoringView),
	m_featureMenu( new QMenu( this ) ),
	m_computerManager( nullptr ),
	m_computerListModel( nullptr ),
	m_featureManager( nullptr )
{
	ui->setupUi( this );

	connect( ui->listView, &QListView::customContextMenuRequested,
			 this, &ComputerMonitoringView::showContextMenu );
	connect( ui->gridSizeSlider, &QSlider::valueChanged,
			 this, &ComputerMonitoringView::setComputerScreenSize );
}



ComputerMonitoringView::~ComputerMonitoringView()
{
	delete ui;

	delete m_computerListModel;
}



void ComputerMonitoringView::setConfiguration(PersonalConfig &config)
{
	m_config = &config;

	if( m_config->monitoringScreenSize() >= ui->gridSizeSlider->minimum() )
	{
		ui->gridSizeSlider->setValue( m_config->monitoringScreenSize() );
	}
	else
	{
		ui->gridSizeSlider->setValue( DefaultComputerScreenSize );
	}
}



void ComputerMonitoringView::setComputerManager( ComputerManager &computerManager )
{
	delete m_computerListModel;

	m_computerManager = &computerManager;

	m_computerListModel = new ComputerListModel( computerManager, this );

	ui->listView->setModel( m_computerListModel );
}



void ComputerMonitoringView::setFeatureManager( FeatureManager& featureManager )
{
	m_featureManager = &featureManager;

	for( auto feature : featureManager.features() )
	{
		if( feature.type() == Feature::BuiltinService )
		{
			continue;
		}

		m_featureMenu->addAction( QIcon( feature.iconUrl() ),
								  feature.displayName(),
								  [=] () { runFeature( feature ); } );
	}
}



ComputerControlInterfaceList ComputerMonitoringView::selectedComputerControlInterfaces()
{
	ComputerControlInterfaceList computerControlInterfaces;

	if( m_computerListModel )
	{
		for( auto index : ui->listView->selectionModel()->selectedIndexes() )
		{
			computerControlInterfaces += &( m_computerListModel->computerControlInterface( index ) );
		}
	}

	return computerControlInterfaces;
}



void ComputerMonitoringView::showContextMenu( const QPoint& pos )
{
	m_featureMenu->exec( ui->listView->mapToGlobal( pos ) );
}



void ComputerMonitoringView::setComputerScreenSize( int size )
{
	if( m_computerManager && m_config )
	{
		m_config->setMonitoringScreenSize( size );

		m_computerManager->updateComputerScreenSize();
	}
}



void ComputerMonitoringView::runFeature( const Feature& feature )
{
	m_featureManager->startMasterFeature( feature, selectedComputerControlInterfaces(), topLevelWidget() );
}
