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
#include <QScrollBar>

#include "ComputerManager.h"
#include "ComputerMonitoringView.h"
#include "MasterCore.h"
#include "FeatureManager.h"
#include "ItalcConfiguration.h"
#include "UserConfig.h"

#include "ui_ComputerMonitoringView.h"

ComputerMonitoringView::ComputerMonitoringView( QWidget *parent ) :
	QWidget(parent),
	ui(new Ui::ComputerMonitoringView),
	m_masterCore( nullptr ),
	m_featureMenu( new QMenu( this ) ),
	m_computerListModel( nullptr )
{
	ui->setupUi( this );

	connect( ui->listView, &QListView::doubleClicked,
			 this, &ComputerMonitoringView::runDoubleClickFeature );

	connect( ui->listView, &QListView::customContextMenuRequested,
			 this, &ComputerMonitoringView::showContextMenu );
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

	// create computer list model and attach it to list view
	m_computerListModel = new ComputerListModel( m_masterCore->computerManager(), this );

	ui->listView->setModel( m_computerListModel );

	// populate feature menu
	Plugin::Uid previousPluginUid;

	for( const auto& feature : m_masterCore->features() )
	{
		Plugin::Uid pluginUid = m_masterCore->featureManager().pluginUid( feature );

		if( previousPluginUid.isNull() == false &&
				pluginUid != previousPluginUid &&
				feature.testFlag( Feature::Mode ) == false )
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



void ComputerMonitoringView::runDoubleClickFeature( const QModelIndex& index )
{
	const Feature& feature = m_masterCore->featureManager().feature( ItalcCore::config().computerDoubleClickFeature() );

	if( index.isValid() && feature.isValid() )
	{
		ui->listView->selectionModel()->select( index, QItemSelectionModel::SelectCurrent );
		runFeature( feature );
	}
}



void ComputerMonitoringView::showContextMenu( const QPoint& pos )
{
	m_featureMenu->exec( ui->listView->mapToGlobal( pos ) );
}



void ComputerMonitoringView::setComputerScreenSize( int size )
{
	if( m_masterCore )
	{
		m_masterCore->userConfig().setMonitoringScreenSize( size );

		m_masterCore->computerManager().updateComputerScreenSize();

		ui->listView->setIconSize( QSize( size, size * 9 / 16 ) );
	}
}



void ComputerMonitoringView::autoAdjustComputerScreenSize()
{
	int size = ui->listView->iconSize().width();

	if( ui->listView->verticalScrollBar()->isVisible() )
	{
		while( ui->listView->verticalScrollBar()->isVisible() )
		{
			size -= 10;
			setComputerScreenSize( size );
			QApplication::processEvents();
		}
	}
	else
	{
		while( ui->listView->verticalScrollBar()->isVisible() == false )
		{
			size += 10;
			setComputerScreenSize( size );
			QApplication::processEvents();
		}

		setComputerScreenSize( size-20 );
	}
}



void ComputerMonitoringView::runFeature( const Feature& feature )
{
	if( m_masterCore == nullptr )
	{
		return;
	}

	ComputerControlInterfaceList computerControlInterfaces = selectedComputerControlInterfaces();

	// stop any active mode feature
	if( feature.testFlag( Feature::Mode ) )
	{
		for( const auto& currentFeature : m_masterCore->features() )
		{
			if( currentFeature.testFlag( Feature::Mode ) && currentFeature != feature )
			{
				m_masterCore->featureManager().stopMasterFeature( currentFeature, computerControlInterfaces,
																  m_masterCore->localComputerControlInterface(), topLevelWidget() );
			}
		}
	}

	m_masterCore->featureManager().startMasterFeature( feature, computerControlInterfaces,
													   m_masterCore->localComputerControlInterface(), topLevelWidget() );
}
