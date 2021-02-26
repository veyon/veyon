/*
 * SpotlightModel.cpp - implementation of SpotlightModel
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

#include "SpotlightModel.h"


SpotlightModel::SpotlightModel( QAbstractItemModel* sourceModel, QObject* parent ) :
	QSortFilterProxyModel( parent )
{
	setSourceModel( sourceModel );
}



void SpotlightModel::setIconSize( QSize size )
{
	m_iconSize = size;

	Q_EMIT dataChanged( index( 0, 0 ), index( rowCount() - 1, 0 ), { Qt::DisplayRole, Qt::DecorationRole } );
}



void SpotlightModel::setUpdateInRealtime( bool enabled )
{
	m_updateInRealtime = enabled;

	for( const auto& controlInterface : m_controlInterfaces )
	{
		controlInterface->setUpdateMode( m_updateInRealtime
											 ? ComputerControlInterface::UpdateMode::Live
											 : ComputerControlInterface::UpdateMode::Monitoring );
	}
}



void SpotlightModel::add( const ComputerControlInterface::Pointer& controlInterface )
{
	m_controlInterfaces.append( controlInterface );

	controlInterface->setUpdateMode( m_updateInRealtime
										 ? ComputerControlInterface::UpdateMode::Live
										 : ComputerControlInterface::UpdateMode::Monitoring );

	invalidateFilter();
}



void SpotlightModel::remove( const ComputerControlInterface::Pointer& controlInterface )
{
	m_controlInterfaces.removeAll( controlInterface );

	controlInterface->setUpdateMode( ComputerControlInterface::UpdateMode::Monitoring );

	invalidateFilter();
}



QVariant SpotlightModel::data( const QModelIndex& index, int role ) const
{
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



bool SpotlightModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
	return m_controlInterfaces.contains( sourceModel()->data( sourceModel()->index( sourceRow, 0, sourceParent ),
															  ComputerListModel::ControlInterfaceRole )
											 .value<ComputerControlInterface::Pointer>() );
}
