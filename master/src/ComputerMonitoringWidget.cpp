/*
 * ComputerMonitoringWidget.cpp - provides a view with computer monitor thumbnails
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

#include <QApplication>
#include <QMenu>
#include <QScrollBar>
#include <QShowEvent>
#include <QTimer>

#include "ComputerControlListModel.h"
#include "ComputerMonitoringWidget.h"
#include "ComputerMonitoringModel.h"
#include "VeyonMaster.h"
#include "FeatureManager.h"
#include "VeyonConfiguration.h"


ComputerMonitoringWidget::ComputerMonitoringWidget( QWidget *parent ) :
	FlexibleListView( parent ),
	m_featureMenu( new QMenu( this ) )
{
	setContextMenuPolicy( Qt::CustomContextMenu );
	setAcceptDrops( true );
	setDragEnabled( true );
	setDragDropMode( QAbstractItemView::DropOnly );
	setDefaultDropAction( Qt::MoveAction );
	setSelectionMode( QAbstractItemView::ExtendedSelection );
	setFlow( QListView::LeftToRight );
	setWrapping( true );
	setResizeMode( QListView::Adjust );
	setSpacing( 16 );
	setViewMode( QListView::IconMode );
	setUniformItemSizes( true );
	setSelectionRectVisible( true );

	setUidRole( ComputerControlListModel::UidRole );

	connect( this, &QListView::doubleClicked, this, &ComputerMonitoringWidget::runDoubleClickFeature );
	connect( this, &QListView::customContextMenuRequested,
			 this, [this]( const QPoint& pos ) { showContextMenu( mapToGlobal( pos ) ); } );

	initializeView();

	setModel( listModel() );
}



ComputerControlInterfaceList ComputerMonitoringWidget::selectedComputerControlInterfaces() const
{
	const auto& computerControlListModel = master()->computerControlListModel();
	ComputerControlInterfaceList computerControlInterfaces;

	const auto selectedIndices = selectionModel()->selectedIndexes();
	computerControlInterfaces.reserve( selectedIndices.size() );

	for( const auto& index : selectedIndices )
	{
		const auto sourceIndex = listModel()->mapToSource( index );
		computerControlInterfaces.append( computerControlListModel.computerControlInterface( sourceIndex ) );
	}

	return computerControlInterfaces;
}



void ComputerMonitoringWidget::autoAdjustComputerScreenSize()
{
	int size = iconSize().width();

	if( verticalScrollBar()->isVisible() ||
		horizontalScrollBar()->isVisible() )
	{
		while( ( verticalScrollBar()->isVisible() ||
				 horizontalScrollBar()->isVisible() ) &&
			   size > MinimumComputerScreenSize )
		{
			size -= 10;
			setComputerScreenSize( size );
			QApplication::processEvents();
		}
	}
	else
	{
		while( verticalScrollBar()->isVisible() == false &&
			   horizontalScrollBar()->isVisible() == false &&
			   size < MaximumComputerScreenSize )
		{
			size += 10;
			setComputerScreenSize( size );
			QApplication::processEvents();
		}

		setComputerScreenSize( size-20 );
	}

	emit computerScreenSizeAdjusted( size );
}



void ComputerMonitoringWidget::setUseCustomComputerPositions( bool enabled )
{
	setFlexible( enabled );
}



void ComputerMonitoringWidget::alignComputers()
{
	alignToGrid();
}



void ComputerMonitoringWidget::showContextMenu( QPoint globalPos )
{
	populateFeatureMenu( activeFeatures( selectedComputerControlInterfaces() ) );

	m_featureMenu->exec( globalPos );
}



void ComputerMonitoringWidget::setColors( const QColor& backgroundColor, const QColor& textColor )
{
	auto pal = palette();
	pal.setColor( QPalette::Base, backgroundColor );
	pal.setColor( QPalette::Text, textColor );
	setPalette( pal );
}



QJsonArray ComputerMonitoringWidget::saveComputerPositions()
{
	return savePositions();
}



bool ComputerMonitoringWidget::useCustomComputerPositions()
{
	return flexible();
}



void ComputerMonitoringWidget::loadComputerPositions( const QJsonArray& positions )
{
	loadPositions( positions );
}



void ComputerMonitoringWidget::setIconSize( const QSize& size )
{
	QAbstractItemView::setIconSize( size );
}



void ComputerMonitoringWidget::populateFeatureMenu( const FeatureUidList& activeFeatures )
{
	Plugin::Uid previousPluginUid;

	m_featureMenu->clear();

	for( const auto& feature : master()->features() )
	{
		Plugin::Uid pluginUid = master()->featureManager().pluginUid( feature );

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

		const auto subFeatures = master()->subFeatures( feature.uid() );

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



void ComputerMonitoringWidget::addFeatureToMenu( const Feature& feature, const QString& label )
{
	m_featureMenu->addAction( QIcon( feature.iconUrl() ),
							  label,
							  m_featureMenu, [=] () { runFeature( feature ); } );
}



void ComputerMonitoringWidget::addSubFeaturesToMenu( const Feature& parentFeature, const FeatureList& subFeatures, const QString& label )
{
	auto menu = m_featureMenu->addMenu( QIcon( parentFeature.iconUrl() ), label );

	for( const auto& subFeature : subFeatures )
	{
		menu->addAction( QIcon( subFeature.iconUrl() ), subFeature.displayName(), m_featureMenu,
						 [=]() { runFeature( subFeature ); }, subFeature.shortcut() );
	}
}



void ComputerMonitoringWidget::runDoubleClickFeature( const QModelIndex& index )
{
	const Feature& feature = master()->featureManager().feature( VeyonCore::config().computerDoubleClickFeature() );

	if( index.isValid() && feature.isValid() )
	{
		selectionModel()->select( index, QItemSelectionModel::SelectCurrent );
		runFeature( feature );
	}
}



void ComputerMonitoringWidget::showEvent( QShowEvent* event )
{
	if( event->spontaneous() == false &&
		VeyonCore::config().autoAdjustGridSize() )
	{
		QTimer::singleShot( 250, this, &ComputerMonitoringWidget::autoAdjustComputerScreenSize );
	}

	FlexibleListView::showEvent( event );
}



void ComputerMonitoringWidget::wheelEvent( QWheelEvent* event )
{
	if( event->modifiers().testFlag( Qt::ControlModifier ) )
	{
		setComputerScreenSize( iconSize().width() + event->angleDelta().y() / 8 );

		emit computerScreenSizeAdjusted( computerScreenSize() );

		event->accept();
	}
	else
	{
		QWidget::wheelEvent( event );
	}
}
