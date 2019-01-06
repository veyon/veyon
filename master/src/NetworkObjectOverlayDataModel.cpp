/*
 * NetworkObjectOverlayDataModel.cpp - overlay model for NetworkObjectModel to provide extra data
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

#include "NetworkObjectOverlayDataModel.h"
#include "NetworkObjectModel.h"


NetworkObjectOverlayDataModel::NetworkObjectOverlayDataModel( int overlayDataColumn,
															  int overlayDataRole,
															  const QVariant& overlayDataColumnHeaderData,
															  QObject *parent ) :
	QIdentityProxyModel( parent ),
	m_overlayDataColumn( overlayDataColumn ),
	m_overlayDataRole( overlayDataRole ),
	m_overlayDataColumnHeaderData( overlayDataColumnHeaderData )
{
}



int NetworkObjectOverlayDataModel::columnCount( const QModelIndex& parent ) const
{
	return qMax<int>( QIdentityProxyModel::columnCount( parent ), m_overlayDataColumn + 1 );
}



QVariant NetworkObjectOverlayDataModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
	if( section == m_overlayDataColumn && role == Qt::DisplayRole && orientation == Qt::Horizontal )
	{
		return m_overlayDataColumnHeaderData;
	}

	return QIdentityProxyModel::headerData( section, orientation, role );
}



QVariant NetworkObjectOverlayDataModel::data( const QModelIndex& index, int role ) const
{
	if( index.column() != m_overlayDataColumn || role != m_overlayDataRole )
	{
		return QIdentityProxyModel::data( index, role );
	}

	NetworkObject::Uid networkObjectUid = data( index, NetworkObjectModel::UidRole ).toUuid();

	if( networkObjectUid.isNull() == false && m_overlayData.contains( networkObjectUid ) )
	{
		return m_overlayData[networkObjectUid];
	}

	return QVariant();
}



bool NetworkObjectOverlayDataModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
	if( index.column() != m_overlayDataColumn || role != m_overlayDataRole )
	{
		return QIdentityProxyModel::setData( index, value, role );
	}

	NetworkObject::Uid networkObjectUid = data( index, NetworkObjectModel::UidRole ).toUuid();

	if( m_overlayData[networkObjectUid] != value )
	{
		m_overlayData[networkObjectUid] = value;
		emit dataChanged( index, index, QVector<int>( { m_overlayDataRole } ) );
	}

	return true;
}
