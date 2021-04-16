/*
 * ComputerMonitoringWidget.cpp - provides a view with computer monitor thumbnails
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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
	const auto computerMonitoringThumbnailSpacing = VeyonCore::config().computerMonitoringThumbnailSpacing();

	setContextMenuPolicy( Qt::CustomContextMenu );
	setAcceptDrops( true );
	setDragEnabled( true );
	setDragDropMode( QAbstractItemView::DropOnly );
	setDefaultDropAction( Qt::MoveAction );
	setSelectionMode( QAbstractItemView::ExtendedSelection );
	setFlow( QListView::LeftToRight );
	setWrapping( true );
	setResizeMode( QListView::Adjust );
	setSpacing( computerMonitoringThumbnailSpacing  );
	setViewMode( QListView::IconMode );
	setUniformItemSizes( true );
	setSelectionRectVisible( true );

	setUidRole( ComputerControlListModel::UidRole );

	connect( this, &QListView::doubleClicked, this, &ComputerMonitoringWidget::runDoubleClickFeature );
	connect( this, &QListView::customContextMenuRequested,
			 this, [this]( QPoint pos ) { showContextMenu( mapToGlobal( pos ) ); } );

	initializeView( this );

	setModel( dataModel() );
}



ComputerControlInterfaceList ComputerMonitoringWidget::selectedComputerControlInterfaces() const
{
	ComputerControlInterfaceList computerControlInterfaces;

	const auto selectedIndices = selectionModel()->selectedIndexes(); // clazy:exclude=inefficient-qlist
	computerControlInterfaces.reserve( selectedIndices.size() );

	for( const auto& index : selectedIndices )
	{
		computerControlInterfaces.append( model()->data( index, ComputerControlListModel::ControlInterfaceRole )
											  .value<ComputerControlInterface::Pointer>() );
	}

	return computerControlInterfaces;
}



bool ComputerMonitoringWidget::performIconSizeAutoAdjust()
{
	if( ComputerMonitoringView::performIconSizeAutoAdjust() == false)
	{
		return false;
	}

	m_ignoreResizeEvent = true;

	auto size = iconSize().width();

	setComputerScreenSize( size );
	QApplication::processEvents();

	while( verticalScrollBar()->isVisible() == false &&
		   horizontalScrollBar()->isVisible() == false &&
		   size < MaximumComputerScreenSize )
	{
		size += IconSizeAdjustStepSize;
		setComputerScreenSize( size );
		QApplication::processEvents();
	}

	while( ( verticalScrollBar()->isVisible() ||
			 horizontalScrollBar()->isVisible() ) &&
		   size > MinimumComputerScreenSize )
	{
		size -= IconSizeAdjustStepSize;
		setComputerScreenSize( size );
		QApplication::processEvents();
	}

	Q_EMIT computerScreenSizeAdjusted( size );

	m_ignoreResizeEvent = false;

	return true;
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
	populateFeatureMenu( selectedComputerControlInterfaces() );

	m_featureMenu->exec( globalPos );
}



void ComputerMonitoringWidget::setIconSize( const QSize& size )
{
	QAbstractItemView::setIconSize( size );
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



void ComputerMonitoringWidget::populateFeatureMenu(  const ComputerControlInterfaceList& computerControlInterfaces )
{
	Plugin::Uid previousPluginUid;

	m_featureMenu->clear();

	for( const auto& feature : master()->features() )
	{
		if( feature.testFlag( Feature::Meta ) )
		{
			continue;
		}

		Plugin::Uid pluginUid = master()->featureManager().pluginUid( feature );

		if( previousPluginUid.isNull() == false &&
			pluginUid != previousPluginUid &&
			feature.testFlag( Feature::Mode ) == false )
		{
			m_featureMenu->addSeparator();
		}

		previousPluginUid = pluginUid;

		auto active = false;

		auto label = feature.displayName();
		if( feature.displayNameActive().isEmpty() == false &&
			isFeatureOrSubFeatureActive( computerControlInterfaces, feature.uid() ) )
		{
			label = feature.displayNameActive();
			active = true;
		}

		const auto subFeatures = master()->subFeatures( feature.uid() );
		if( subFeatures.isEmpty() || active )
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



void ComputerMonitoringWidget::addSubFeaturesToMenu( const Feature& parentFeature, const FeatureList& subFeatures, const QString& label )
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



void ComputerMonitoringWidget::runDoubleClickFeature( const QModelIndex& index )
{
	const Feature& feature = master()->featureManager().feature( VeyonCore::config().computerDoubleClickFeature() );

	if( index.isValid() && feature.isValid() )
	{
		selectionModel()->select( index, QItemSelectionModel::SelectCurrent );
		runFeature( feature );
	}
}



void ComputerMonitoringWidget::resizeEvent( QResizeEvent* event )
{
	FlexibleListView::resizeEvent( event );

	if( m_ignoreResizeEvent == false )
	{
		initiateIconSizeAutoAdjust();
	}
}



void ComputerMonitoringWidget::showEvent( QShowEvent* event )
{
	if( event->spontaneous() == false )
	{
		initiateIconSizeAutoAdjust();
	}

	FlexibleListView::showEvent( event );
}



void ComputerMonitoringWidget::wheelEvent( QWheelEvent* event )
{
	if( m_ignoreWheelEvent == false &&
		event->modifiers().testFlag( Qt::ControlModifier ) )
	{
		setComputerScreenSize( iconSize().width() + event->angleDelta().y() / 8 );

		Q_EMIT computerScreenSizeAdjusted( computerScreenSize() );

		event->accept();
	}
	else
	{
		QListView::wheelEvent( event );
	}
}
