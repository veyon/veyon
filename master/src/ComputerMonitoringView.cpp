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
#include "MasterCore.h"
#include "FeatureManager.h"
#include "PersonalConfig.h"

#include "ui_ComputerMonitoringView.h"

ComputerMonitoringView::ComputerMonitoringView( QWidget *parent ) :
	QWidget(parent),
	ui(new Ui::ComputerMonitoringView),
	m_masterCore( nullptr ),
	m_featureMenu( new QMenu( this ) ),
	m_computerListModel( nullptr )
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



void ComputerMonitoringView::setMasterCore( MasterCore& masterCore )
{
	if( m_masterCore )
	{
		return;
	}

	m_masterCore = &masterCore;

	// initialize grid size slider
	if( m_masterCore->personalConfig().monitoringScreenSize() >= ui->gridSizeSlider->minimum() )
	{
		ui->gridSizeSlider->setValue( m_masterCore->personalConfig().monitoringScreenSize() );
	}
	else
	{
		ui->gridSizeSlider->setValue( DefaultComputerScreenSize );
	}

	// create computer list model and attach it to list view
	m_computerListModel = new ComputerListModel( m_masterCore->computerManager(), this );

	ui->listView->setModel( m_computerListModel );

	// populate feature menu
	Plugin::Uid previousPluginUid;

	for( auto feature : m_masterCore->features() )
	{
		Plugin::Uid pluginUid = m_masterCore->featureManager().pluginUid( feature );

		if( previousPluginUid.isNull() == false &&
				pluginUid != previousPluginUid &&
				feature.type() != Feature::Mode )
		{
			m_featureMenu->addSeparator();
		}

		previousPluginUid = pluginUid;

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
	if( m_masterCore )
	{
		m_masterCore->personalConfig().setMonitoringScreenSize( size );

		m_masterCore->computerManager().updateComputerScreenSize();
	}
}



void ComputerMonitoringView::runFeature( const Feature& feature )
{
	if( m_masterCore )
	{
		m_masterCore->featureManager().startMasterFeature( feature, selectedComputerControlInterfaces(), topLevelWidget() );
	}
}
