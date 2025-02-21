/*
 * ComputerMonitoringWidget.cpp - provides a view with computer monitor thumbnails
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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
#include "ComputerItemDelegate.h"
#include "ComputerMonitoringModel.h"
#include "ComputerMonitoringWidget.h"
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

	setItemDelegate(new ComputerItemDelegate(this));

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



void ComputerMonitoringWidget::populateFeatureMenu( const ComputerControlInterfaceList& computerControlInterfaces )
{
	Plugin::Uid previousPluginUid;

	m_featureMenu->clear();

	for( const auto& feature : master()->features() )
	{
		if( feature.testFlag( Feature::Flag::Meta ) )
		{
			continue;
		}

		Plugin::Uid pluginUid = VeyonCore::featureManager().pluginUid( feature.uid() );

		if( previousPluginUid.isNull() == false &&
			pluginUid != previousPluginUid &&
			feature.testFlag( Feature::Flag::Mode ) == false )
		{
			m_featureMenu->addSeparator();
		}

		previousPluginUid = pluginUid;

		if( feature.displayNameActive().isEmpty() == false &&
			isFeatureOrRelatedFeatureActive( computerControlInterfaces, feature.uid() ) )
		{
			addFeatureToMenu( feature, feature.displayNameActive() );
		}
		else
		{
			const auto subFeatures = master()->subFeatures( feature.uid() );
			if( subFeatures.isEmpty() )
			{
				addFeatureToMenu( feature, feature.displayName() );
			}
			else
			{
				addSubFeaturesToMenu( feature, subFeatures, feature.displayName() );
			}
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
		menu->addAction(QIcon(subFeature.iconUrl()), subFeature.displayName(),
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
						subFeature.shortcut(),
#endif
						this, [=]() { runFeature( subFeature ); }
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
		, subFeature.shortcut()
#endif
		);
	}
}



void ComputerMonitoringWidget::runDoubleClickFeature( const QModelIndex& index )
{
	const Feature& feature = VeyonCore::featureManager().feature( VeyonCore::config().computerDoubleClickFeature() );

	if( index.isValid() && feature.isValid() )
	{
		selectionModel()->select( index, QItemSelectionModel::SelectCurrent );
		runFeature( feature );
	}
}



void ComputerMonitoringWidget::resetIgnoreNumberOfMouseEvents( )
{
	m_ignoreNumberOfMouseEvents = IgnoredNumberOfMouseEventsWhileHold;
}



void ComputerMonitoringWidget::runMousePressAndHoldFeature( )
{
	m_mousePressAndHold.stop();
	const auto selectedInterfaces = selectedComputerControlInterfaces();
	if( !m_ignoreMousePressAndHoldEvent &&
		selectedInterfaces.count() > 0 &&
		selectedInterfaces.count() < 2 &&
		selectedInterfaces.first()->state() == ComputerControlInterface::State::Connected &&
		selectedInterfaces.first()->hasValidFramebuffer() )
	{
		m_ignoreMousePressAndHoldEvent = true;
		resetIgnoreNumberOfMouseEvents();
		delete m_computerZoomWidget;
		m_computerZoomWidget = new ComputerZoomWidget( selectedInterfaces.first() );
		connect( m_computerZoomWidget, &ComputerZoomWidget::keypressInComputerZoomWidget, this, &ComputerMonitoringWidget::resetIgnoreNumberOfMouseEvents );
	}
}



void ComputerMonitoringWidget::stopMousePressAndHoldFeature( )
{
	disconnect( m_computerZoomWidget, &ComputerZoomWidget::keypressInComputerZoomWidget, this, &ComputerMonitoringWidget::resetIgnoreNumberOfMouseEvents );
	m_ignoreMousePressAndHoldEvent = false;
	m_ignoreNumberOfMouseEvents = 0;
	m_computerZoomWidget->close();
	delete m_computerZoomWidget;
	m_computerZoomWidget = nullptr;
}



void ComputerMonitoringWidget::mousePressEvent( QMouseEvent* event )
{
	if( event->buttons() == Qt::LeftButton && indexAt(event->pos()).isValid() )
	{
		if( !m_ignoreMousePressAndHoldEvent )
		{
			m_mousePressAndHold.setInterval( 500 );
			m_mousePressAndHold.start();
			connect(&m_mousePressAndHold, &QTimer::timeout, this, &ComputerMonitoringWidget::runMousePressAndHoldFeature );
		}
	}
	QListView::mousePressEvent( event );
}



void ComputerMonitoringWidget::mouseReleaseEvent( QMouseEvent* event )
{
	m_mousePressAndHold.stop();
	if ( m_ignoreMousePressAndHoldEvent )
	{
		stopMousePressAndHoldFeature();
	}
	QListView::mouseReleaseEvent( event );
}



void ComputerMonitoringWidget::mouseMoveEvent( QMouseEvent* event )
{
	m_mousePressAndHold.stop();
	if ( m_ignoreNumberOfMouseEvents <= 0 )
	{
		if ( m_ignoreMousePressAndHoldEvent )
		{
			stopMousePressAndHoldFeature();
		}

		QListView::mouseMoveEvent( event );
	} else
	{
		m_ignoreNumberOfMouseEvents--;
		event->accept();
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
