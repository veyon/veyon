/*
 * ComputerMonitoringView.cpp - provides a view with computer monitor thumbnails
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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
#include "ComputerSortFilterProxyModel.h"
#include "VeyonMaster.h"
#include "FeatureManager.h"
#include "VeyonConfiguration.h"
#include "UserConfig.h"

#include "ui_ComputerMonitoringView.h"

ComputerMonitoringView::ComputerMonitoringView( QWidget *parent ) :
	QWidget(parent),
	ui(new Ui::ComputerMonitoringView),
	m_master( nullptr ),
	m_featureMenu( new QMenu( this ) )
{
	ui->setupUi( this );

	ui->listView->setUidRole( ComputerControlListModel::UidRole );

	connect( ui->listView, &QListView::doubleClicked,
			 this, &ComputerMonitoringView::runDoubleClickFeature );

	connect( ui->listView, &QListView::customContextMenuRequested,
			 this, &ComputerMonitoringView::showContextMenu );
}



ComputerMonitoringView::~ComputerMonitoringView()
{
	if( m_master )
	{
		m_master->userConfig().setFilterPoweredOnComputers( listModel().stateFilter() != ComputerControlInterface::None );
		m_master->userConfig().setComputerPositions( ui->listView->savePositions() );
		m_master->userConfig().setUseCustomComputerPositions( ui->listView->flexible() );
	}

	delete ui;
}



void ComputerMonitoringView::setVeyonMaster( VeyonMaster& masterCore )
{
	if( m_master )
	{
		return;
	}

	m_master = &masterCore;

	auto palette = ui->listView->palette();
	palette.setColor( QPalette::Base, VeyonCore::config().computerMonitoringBackgroundColor() );
	palette.setColor( QPalette::Text, VeyonCore::config().computerMonitoringTextColor() );
	ui->listView->setPalette( palette );

	// attach proxy model to view
	ui->listView->setModel( &listModel() );

	// load custom positions
	ui->listView->loadPositions( m_master->userConfig().computerPositions() );
	ui->listView->setFlexible( m_master->userConfig().useCustomComputerPositions() );
}



ComputerControlInterfaceList ComputerMonitoringView::selectedComputerControlInterfaces()
{
	const auto& computerControlListModel = m_master->computerControlListModel();
	ComputerControlInterfaceList computerControlInterfaces;

	const auto selectedIndices = ui->listView->selectionModel()->selectedIndexes();
	computerControlInterfaces.reserve( selectedIndices.size() );

	for( const auto& index : selectedIndices )
	{
		const auto sourceIndex = listModel().mapToSource( index );
		computerControlInterfaces.append( computerControlListModel.computerControlInterface( sourceIndex ) );
	}

	return computerControlInterfaces;
}



void ComputerMonitoringView::setSearchFilter( const QString& searchFilter )
{
	listModel().setFilterRegExp( searchFilter );
}



void ComputerMonitoringView::setFilterPoweredOnComputers( bool enabled )
{
	listModel().setStateFilter( enabled ? ComputerControlInterface::Connected : ComputerControlInterface::None );
}



void ComputerMonitoringView::runDoubleClickFeature( const QModelIndex& index )
{
	const Feature& feature = m_master->featureManager().feature( VeyonCore::config().computerDoubleClickFeature() );

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
	if( m_master )
	{
		m_master->userConfig().setMonitoringScreenSize( size );

		m_master->computerControlListModel().updateComputerScreenSize();

		ui->listView->setIconSize( QSize( size, size * 9 / 16 ) );
	}
}



void ComputerMonitoringView::autoAdjustComputerScreenSize()
{
	int size = ui->listView->iconSize().width();

	if( ui->listView->verticalScrollBar()->isVisible() ||
			ui->listView->horizontalScrollBar()->isVisible() )
	{
		while( ( ui->listView->verticalScrollBar()->isVisible() ||
				 ui->listView->horizontalScrollBar()->isVisible() ) &&
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
			   ui->listView->horizontalScrollBar()->isVisible() == false &&
			   size < MaximumComputerScreenSize )
		{
			size += 10;
			setComputerScreenSize( size );
			QApplication::processEvents();
		}

		setComputerScreenSize( size-20 );
	}

	emit computerScreenSizeAdjusted( m_master->userConfig().monitoringScreenSize() );
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
	if( m_master == nullptr )
	{
		return;
	}

	ComputerControlInterfaceList computerControlInterfaces = selectedComputerControlInterfaces();

	// mode feature already active?
	if( feature.testFlag( Feature::Mode ) &&
			activeFeatures( computerControlInterfaces ).contains( feature.uid().toString() ) )
	{
		// then stop it
		m_master->featureManager().stopFeature( *m_master, feature, computerControlInterfaces );
	}
	else
	{
		// stop all other active mode feature
		if( feature.testFlag( Feature::Mode ) )
		{
			for( const auto& currentFeature : m_master->features() )
			{
				if( currentFeature.testFlag( Feature::Mode ) && currentFeature != feature )
				{
					m_master->featureManager().stopFeature( *m_master, currentFeature, computerControlInterfaces );
				}
			}
		}

		m_master->featureManager().startFeature( *m_master, feature, computerControlInterfaces );
	}
}



ComputerSortFilterProxyModel& ComputerMonitoringView::listModel()
{
	return m_master->computerSortFilterProxyModel();
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

		emit computerScreenSizeAdjusted( m_master->userConfig().monitoringScreenSize() );

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

	for( const auto& feature : m_master->features() )
	{
		Plugin::Uid pluginUid = m_master->featureManager().pluginUid( feature );

		if( previousPluginUid.isNull() == false &&
				pluginUid != previousPluginUid &&
				feature.testFlag( Feature::Mode ) == false )
		{
			m_featureMenu->addSeparator();
		}

		previousPluginUid = pluginUid;

		auto label = feature.displayName();
		if( activeFeatures.contains( feature.uid().toString() ) &&
				feature.displayNameActive().isEmpty() == false )
		{
			label = feature.displayNameActive();
		}

		const auto subFeatures = m_master->subFeatures( feature.uid() );

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
