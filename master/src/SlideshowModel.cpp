/*
 * SlideshowModel.cpp - implementation of SlideshowModel
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

#include "ComputerListModel.h"
#include "SlideshowModel.h"


SlideshowModel::SlideshowModel( QAbstractItemModel* sourceModel, QObject* parent ) :
	QSortFilterProxyModel( parent )
{
	setSourceModel( sourceModel );

	connect( sourceModel, &QAbstractItemModel::rowsInserted, this,
			 [this]( const QModelIndex &parent, int start, int end )
			 {
				 Q_UNUSED(parent)
				 Q_UNUSED(end)
				 if( start <= m_currentRow )
				 {
					 showNext();
				 }
			 } );
	connect( sourceModel, &QAbstractItemModel::rowsRemoved, this,
			 [this]( const QModelIndex &parent, int start, int end )
			 {
				 Q_UNUSED(parent)
				 Q_UNUSED(end)
				 if( start <= m_currentRow )
				 {
					 showPrevious();
				 }
			 } );

	connect( &m_timer, &QTimer::timeout, this, &SlideshowModel::showNext );
}



void SlideshowModel::setIconSize( QSize size )
{
	m_iconSize = size;

	Q_EMIT dataChanged( index( 0, 0 ), index( rowCount() - 1, 0 ), { Qt::DisplayRole, Qt::DecorationRole } );
}



QVariant SlideshowModel::data( const QModelIndex& index, int role ) const
{
	if( m_currentControlInterface.isNull() )
	{
		return {};
	}

	const auto sourceIndex = mapToSource( index );
	if( sourceIndex.isValid() == false )
	{
		return {};
	}

	if( role == Qt::DecorationRole )
	{
		auto screen = sourceModel()->data( sourceIndex, ComputerListModel::ScreenRole ).value<QImage>();
		if( screen.isNull() )
		{
			screen = sourceModel()->data( sourceIndex, Qt::DecorationRole ).value<QImage>();
		}

		return screen.scaled( m_iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation );
	}

	return QSortFilterProxyModel::data( index, role );
}



void SlideshowModel::setRunning( bool running, int duration )
{
	m_timer.stop();
	m_timer.setInterval( duration );

	if( running )
	{
		m_timer.start();
	}
}



void SlideshowModel::showPrevious()
{
	auto valid = false;

	for( int i = 0; i < sourceModel()->rowCount(); ++i )
	{
		setCurrentRow( m_currentRow - 1 );

		if( m_currentControlInterface &&
			m_currentControlInterface->state() == ComputerControlInterface::State::Connected &&
			m_currentControlInterface->hasValidFramebuffer() )
		{
			valid = true;
			break;
		}
	}

	if( valid == false )
	{
		m_currentRow = 0;
		m_currentControlInterface.clear();
	}

	if( m_timer.isActive() )
	{
		m_timer.stop();
		m_timer.start();
	}
}



void SlideshowModel::showNext()
{
	auto valid = false;

	for( int i = 0; i < sourceModel()->rowCount(); ++i )
	{
		setCurrentRow( m_currentRow + 1 );

		if( m_currentControlInterface &&
			m_currentControlInterface->state() == ComputerControlInterface::State::Connected &&
			m_currentControlInterface->hasValidFramebuffer() )
		{
			valid = true;
			break;
		}
	}

	if( valid == false )
	{
		m_currentRow = 0;
		m_currentControlInterface.clear();
	}

	if( m_timer.isActive() )
	{
		m_timer.stop();
		m_timer.start();
	}
}



bool SlideshowModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
	return sourceRow < sourceModel()->rowCount() &&
		   m_currentControlInterface == sourceModel()->data( sourceModel()->index( sourceRow, 0, sourceParent ),
															 ComputerListModel::ControlInterfaceRole )
											.value<ComputerControlInterface::Pointer>();
}



void SlideshowModel::setCurrentRow( int row )
{
	if( sourceModel()->rowCount() > 0 )
	{
		m_currentRow = qMax( 0, row ) % qMax( 1, sourceModel()->rowCount() );

		m_currentControlInterface = sourceModel()->data( sourceModel()->index( m_currentRow, 0 ),
														 ComputerListModel::ControlInterfaceRole )
										.value<ComputerControlInterface::Pointer>();
	}
	else
	{
		m_currentRow = 0;
		m_currentControlInterface.clear();
	}

	invalidateFilter();
}
