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

#include <QMenu>
#include <QScrollBar>
#include <QShowEvent>
#include <QTimer>

#include "ComputerControlListModel.h"
#include "ComputerManager.h"
#include "ComputerMonitoringWidget.h"
#include "ComputerSortFilterProxyModel.h"
#include "VeyonMaster.h"
#include "FeatureManager.h"
#include "VeyonConfiguration.h"
#include "UserConfig.h"

#include "ui_ComputerMonitoringWidget.h"

ComputerMonitoringWidget::ComputerMonitoringWidget( QWidget *parent ) :
	QWidget(parent),
	ui(new Ui::ComputerMonitoringWidget),
	m_master( VeyonCore::instance()->findChild<VeyonMaster *>() ),
	m_featureMenu( new QMenu( this ) )
{
	Q_ASSERT(m_master != nullptr);

	ui->setupUi( this );

	ui->listView->setUidRole( ComputerControlListModel::UidRole );

	connect( ui->listView, &QListView::doubleClicked,
			 this, &ComputerMonitoringWidget::runDoubleClickFeature );

	connect( ui->listView, &QListView::customContextMenuRequested,
			 this, &ComputerMonitoringWidget::showContextMenu );
}



ComputerMonitoringWidget::~ComputerMonitoringWidget()
{
	if( m_master )
	{
		m_master->userConfig().setFilterPoweredOnComputers( dataModel().stateFilter() != ComputerControlInterface::State::None );
		m_master->userConfig().setComputerPositions( ui->listView->savePositions() );
		m_master->userConfig().setUseCustomComputerPositions( ui->listView->flexible() );
	}

	delete ui;
}



void ComputerMonitoringWidget::loadSettings()
{
	auto palette = ui->listView->palette();
	palette.setColor( QPalette::Base, VeyonCore::config().computerMonitoringBackgroundColor() );
	palette.setColor( QPalette::Text, VeyonCore::config().computerMonitoringTextColor() );
	ui->listView->setPalette( palette );

	// attach proxy model to view
	ui->listView->setModel( &dataModel() );

	// load custom positions
	ui->listView->loadPositions( m_master->userConfig().computerPositions() );
	ui->listView->setFlexible( m_master->userConfig().useCustomComputerPositions() );
}



ComputerControlInterfaceList ComputerMonitoringWidget::selectedComputerControlInterfaces()
{
	ComputerControlInterfaceList computerControlInterfaces;

	const auto model = ui->listView->model();
	const auto selectedIndices = ui->listView->selectionModel()->selectedIndexes();
	computerControlInterfaces.reserve( selectedIndices.size() );

	for( const auto& index : selectedIndices )
	{
		computerControlInterfaces.append( model->data( index, ComputerControlListModel::ControlInterfaceRole )
											  .value<ComputerControlInterface::Pointer>() );
	}

	return computerControlInterfaces;
}



void ComputerMonitoringWidget::setSearchFilter( const QString& searchFilter )
{
	dataModel().setFilterRegExp( searchFilter );
}



void ComputerMonitoringWidget::setFilterPoweredOnComputers( bool enabled )
{
	dataModel().setStateFilter( enabled ? ComputerControlInterface::State::Connected : ComputerControlInterface::State::None );
}



void ComputerMonitoringWidget::setComputerScreenSize( int size )
{
	if( m_master )
	{
		m_master->userConfig().setMonitoringScreenSize( size );

		m_master->computerControlListModel().updateComputerScreenSize();

		ui->listView->setIconSize( QSize( size, size * 9 / 16 ) );
	}
}



void ComputerMonitoringWidget::autoAdjustComputerScreenSize()
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

	Q_EMIT computerScreenSizeAdjusted( m_master->userConfig().monitoringScreenSize() );
}



void ComputerMonitoringWidget::setUseCustomComputerPositions( bool enabled )
{
	ui->listView->setFlexible( enabled );
}



void ComputerMonitoringWidget::alignComputers()
{
	ui->listView->alignToGrid();
}



void ComputerMonitoringWidget::showContextMenu( QPoint pos )
{
	populateFeatureMenu( selectedComputerControlInterfaces() );

	m_featureMenu->exec( ui->listView->mapToGlobal( pos ) );
}



void ComputerMonitoringWidget::runDoubleClickFeature( const QModelIndex& index )
{
	const Feature& feature = m_master->featureManager().feature( VeyonCore::config().computerDoubleClickFeature() );

	if( index.isValid() && feature.isValid() )
	{
		ui->listView->selectionModel()->select( index, QItemSelectionModel::SelectCurrent );
		runFeature( feature );
	}
}



void ComputerMonitoringWidget::runFeature( const Feature& feature )
{
	if( m_master == nullptr )
	{
		return;
	}

	ComputerControlInterfaceList computerControlInterfaces = selectedComputerControlInterfaces();

	// mode feature already active?
	if( feature.testFlag( Feature::Mode ) &&
		isFeatureOrSubFeatureActive( computerControlInterfaces, feature.uid() ) )
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



ComputerSortFilterProxyModel& ComputerMonitoringWidget::dataModel()
{
	return m_master->computerSortFilterProxyModel();
}



QListView* ComputerMonitoringWidget::listView() const
{
	return ui->listView;
}



void ComputerMonitoringWidget::showEvent( QShowEvent* event )
{
	if( event->spontaneous() == false &&
			VeyonCore::config().autoAdjustGridSize() )
	{
		QTimer::singleShot( 250, this, &ComputerMonitoringWidget::autoAdjustComputerScreenSize );
	}

	QWidget::showEvent( event );
}



void ComputerMonitoringWidget::wheelEvent( QWheelEvent* event )
{
	if( m_ignoreWheelEvent == false &&
		event->modifiers().testFlag( Qt::ControlModifier ) )
	{
		setComputerScreenSize( qBound<int>( MinimumComputerScreenSize,
											ui->listView->iconSize().width() + event->angleDelta().y() / 8,
											MaximumComputerScreenSize ) );

		Q_EMIT computerScreenSizeAdjusted( m_master->userConfig().monitoringScreenSize() );

		event->accept();
	}
	else
	{
		QWidget::wheelEvent( event );
	}
}



bool ComputerMonitoringWidget::isFeatureOrSubFeatureActive( const ComputerControlInterfaceList& computerControlInterfaces,
														 Feature::Uid featureUid ) const
{
	const auto featureList = FeatureUidList{ featureUid } + m_master->subFeaturesUids( featureUid );

	for( const auto& controlInterface : computerControlInterfaces )
	{
		for( const auto& activeFeature : controlInterface->activeFeatures() )
		{
			if( featureList.contains( activeFeature ) )
			{
				return true;
			}
		}
	}

	return false;
}



void ComputerMonitoringWidget::populateFeatureMenu(  const ComputerControlInterfaceList& computerControlInterfaces )
{
	Plugin::Uid previousPluginUid;

	m_featureMenu->clear();

	for( const auto& feature : m_master->features() )
	{
		if( feature.testFlag( Feature::Internal ) )
		{
			continue;
		}

		Plugin::Uid pluginUid = m_master->featureManager().pluginUid( feature );

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

		const auto subFeatures = m_master->subFeatures( feature.uid() );
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
