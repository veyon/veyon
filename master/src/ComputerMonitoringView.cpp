/*
 * ComputerMonitoringView.cpp - provides a view with computer monitor thumbnails
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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
#include <QShowEvent>
#include <QTimer>

#include "ComputerControlListModel.h"
#include "ComputerManager.h"
#include "ComputerMonitoringView.h"
#include "MasterCore.h"
#include "FeatureManager.h"
#include "VeyonConfiguration.h"
#include "UserConfig.h"

#include "ui_ComputerMonitoringView.h"

ComputerMonitoringView::ComputerMonitoringView( QWidget *parent ) :
	QWidget(parent),
	ui(new Ui::ComputerMonitoringView),
	m_masterCore( nullptr ),
	m_featureMenu( new QMenu( this ) ),
	m_sortFilterProxyModel( this )
{
	ui->setupUi( this );

	ui->listView->setUidRole( ComputerControlListModel::UidRole );

	m_sortFilterProxyModel.setFilterCaseSensitivity( Qt::CaseInsensitive );

	connect( ui->listView, &QListView::doubleClicked,
			 this, &ComputerMonitoringView::runDoubleClickFeature );

	connect( ui->listView, &QListView::customContextMenuRequested,
			 this, &ComputerMonitoringView::showContextMenu );
}



ComputerMonitoringView::~ComputerMonitoringView()
{
	if( m_masterCore )
	{
		m_masterCore->userConfig().setComputerPositions( ui->listView->savePositions() );
		m_masterCore->userConfig().setUseCustomComputerPositions( ui->listView->flexible() );
	}

	delete ui;
}



void ComputerMonitoringView::setMasterCore( MasterCore& masterCore )
{
	if( m_masterCore )
	{
		return;
	}

	m_masterCore = &masterCore;

	auto palette = ui->listView->palette();
	palette.setColor( QPalette::Base, VeyonCore::config().computerMonitoringBackgroundColor() );
	ui->listView->setPalette( palette );

	// attach computer list model to proxy model
	m_sortFilterProxyModel.setSourceModel( &m_masterCore->computerControlListModel() );
	m_sortFilterProxyModel.setSortRole( Qt::InitialSortOrderRole );
	m_sortFilterProxyModel.sort( 0 );

	// attach proxy model to view
	ui->listView->setModel( &m_sortFilterProxyModel );

	// load custom positions
	ui->listView->loadPositions( m_masterCore->userConfig().computerPositions() );
	ui->listView->setFlexible( m_masterCore->userConfig().useCustomComputerPositions() );
}



ComputerControlInterfaceList ComputerMonitoringView::selectedComputerControlInterfaces()
{
	const auto& computerControlListModel = m_masterCore->computerControlListModel();
	ComputerControlInterfaceList computerControlInterfaces;

	const auto selectedIndices = ui->listView->selectionModel()->selectedIndexes();
	computerControlInterfaces.reserve( selectedIndices.size() );

	for( const auto& index : selectedIndices )
	{
		const auto sourceIndex = m_sortFilterProxyModel.mapToSource( index );
		computerControlInterfaces.append( computerControlListModel.computerControlInterface( sourceIndex ) );
	}

	return computerControlInterfaces;
}



void ComputerMonitoringView::setSearchFilter( const QString& searchFilter )
{
	m_sortFilterProxyModel.setFilterRegExp( searchFilter );
}



void ComputerMonitoringView::runDoubleClickFeature( const QModelIndex& index )
{
	const Feature& feature = m_masterCore->featureManager().feature( VeyonCore::config().computerDoubleClickFeature() );

	if( index.isValid() && feature.isValid() )
	{
		ui->listView->selectionModel()->select( index, QItemSelectionModel::SelectCurrent );
		runFeature( feature );
	}
}



void ComputerMonitoringView::showContextMenu( QPoint pos )
{
	populateFeatureMenu( activeFeatures( selectedComputerControlInterfaces() ) );

	m_featureMenu->exec( ui->listView->mapToGlobal( pos ) );
}



void ComputerMonitoringView::setComputerScreenSize( int size )
{
	if( m_masterCore )
	{
		m_masterCore->userConfig().setMonitoringScreenSize( size );

		m_masterCore->computerControlListModel().updateComputerScreenSize();

		ui->listView->setIconSize( QSize( size, size * 9 / 16 ) );
	}
}



void ComputerMonitoringView::autoAdjustComputerScreenSize()
{
	int size = ui->listView->iconSize().width();

	if( ui->listView->verticalScrollBar()->isVisible() )
	{
		while( ui->listView->verticalScrollBar()->isVisible() &&
			   size > MinimumComputerScreenSize )
		{
			size -= 10;
			setComputerScreenSize( size );
			QApplication::processEvents();
		}
	}
	else
	{
		while( ui->listView->verticalScrollBar()->isVisible() == false &&
			   size < MaximumComputerScreenSize )
		{
			size += 10;
			setComputerScreenSize( size );
			QApplication::processEvents();
		}

		setComputerScreenSize( size-20 );
	}

	emit computerScreenSizeAdjusted( m_masterCore->userConfig().monitoringScreenSize() );
}



void ComputerMonitoringView::setUseCustomComputerPositions( bool enabled )
{
	ui->listView->setFlexible( enabled );
}



void ComputerMonitoringView::alignComputers()
{
	ui->listView->alignToGrid();
}



void ComputerMonitoringView::runFeature( const Feature& feature )
{
	if( m_masterCore == nullptr )
	{
		return;
	}

	ComputerControlInterfaceList computerControlInterfaces = selectedComputerControlInterfaces();

	// mode feature already active?
	if( feature.testFlag( Feature::Mode ) &&
			activeFeatures( computerControlInterfaces ).contains( feature.uid().toString() ) )
	{
		// then stop it
		m_masterCore->featureManager().stopMasterFeature( feature, computerControlInterfaces, topLevelWidget() );
	}
	else
	{
		// stop all other active mode feature
		if( feature.testFlag( Feature::Mode ) )
		{
			for( const auto& currentFeature : m_masterCore->features() )
			{
				if( currentFeature.testFlag( Feature::Mode ) && currentFeature != feature )
				{
					m_masterCore->featureManager().stopMasterFeature( currentFeature, computerControlInterfaces, topLevelWidget() );
				}
			}
		}

		m_masterCore->featureManager().startMasterFeature( feature, computerControlInterfaces, topLevelWidget() );
	}
}




void ComputerMonitoringView::showEvent( QShowEvent* event )
{
	if( event->spontaneous() == false &&
			VeyonCore::config().autoAdjustGridSize() )
	{
		QTimer::singleShot( 250, this, &ComputerMonitoringView::autoAdjustComputerScreenSize );
	}

	QWidget::showEvent( event );
}



void ComputerMonitoringView::wheelEvent( QWheelEvent* event )
{
	if( event->modifiers().testFlag( Qt::ControlModifier ) )
	{
		setComputerScreenSize( qBound<int>( MinimumComputerScreenSize,
											ui->listView->iconSize().width() + event->angleDelta().y() / 8,
											MaximumComputerScreenSize ) );

		emit computerScreenSizeAdjusted( m_masterCore->userConfig().monitoringScreenSize() );

		event->accept();
	}
	else
	{
		QWidget::wheelEvent( event );
	}
}



FeatureUidList ComputerMonitoringView::activeFeatures( const ComputerControlInterfaceList& computerControlInterfaces )
{
	FeatureUidList featureUidList;

	for( const auto& controlInterface : computerControlInterfaces )
	{
		featureUidList.append( controlInterface->activeFeatures() );
	}

	featureUidList.removeDuplicates();

	return featureUidList;
}



void ComputerMonitoringView::populateFeatureMenu( const FeatureUidList& activeFeatures )
{
	Plugin::Uid previousPluginUid;

	m_featureMenu->clear();

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

		auto label = feature.displayName();
		if( activeFeatures.contains( feature.uid().toString() ) )
		{
			label = feature.displayNameActive();
		}

		const auto subFeatures = m_masterCore->subFeatures( feature.uid() );

		if( subFeatures.isEmpty() )
		{
			addFeatureToMenu( feature, label );
		}
		else
		{
			addSubFeaturesToMenu( feature, subFeatures, label );
		}
	}
}



void ComputerMonitoringView::addFeatureToMenu( const Feature& feature, const QString& label )
{
#if QT_VERSION < 0x050600
#warning Building legacy compat code for unsupported version of Qt
		auto action = m_featureMenu->addAction( QIcon( feature.iconUrl() ), label );
		connect( action, &QAction::triggered, this, [=] () { runFeature( feature ); } );
#else
		m_featureMenu->addAction( QIcon( feature.iconUrl() ),
								  label,
								  this, [=] () { runFeature( feature ); } );
#endif
}



void ComputerMonitoringView::addSubFeaturesToMenu( const Feature& parentFeature, const FeatureList& subFeatures, const QString& label )
{
	auto menu = m_featureMenu->addMenu( QIcon( parentFeature.iconUrl() ), label );

	for( const auto& subFeature : subFeatures )
	{
#if QT_VERSION < 0x050600
#warning Building legacy compat code for unsupported version of Qt
		auto action = menu->addAction( QIcon( subFeature.iconUrl() ), subFeature.displayName() );
		action->setShortcut( subFeature.shortcut() );
		connect( action, &QAction::triggered, this, [=] () { runFeature( subFeature ); } );
#else
		menu->addAction( QIcon( subFeature.iconUrl() ), subFeature.displayName(), this,
						 [=]() { runFeature( subFeature ); }, subFeature.shortcut() );
#endif
	}
}
