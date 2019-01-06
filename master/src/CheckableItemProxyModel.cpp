/*
 * CheckableItemProxyModel.cpp - proxy model for overlaying checked property
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

#include "CheckableItemProxyModel.h"

CheckableItemProxyModel::CheckableItemProxyModel( int uidRole, QObject *parent ) :
	QIdentityProxyModel(parent),
	m_uidRole( uidRole ),
	m_callDepth( 0 )
{
	connect( this, &QIdentityProxyModel::rowsInserted,
			 this, &CheckableItemProxyModel::updateNewRows );
	connect( this, &QIdentityProxyModel::rowsAboutToBeRemoved,
			 this, &CheckableItemProxyModel::removeRowStates );
}



Qt::ItemFlags CheckableItemProxyModel::flags(const QModelIndex &index) const
{
	if( index.isValid() == false || index.column() > 0 )
	{
		return QIdentityProxyModel::flags( index );
	}

	return QIdentityProxyModel::flags( index ) | Qt::ItemIsUserCheckable;
}



QVariant CheckableItemProxyModel::data(const QModelIndex &index, int role) const
{
	if( !index.isValid() )
	{
		return QVariant();
	}

	if( role == Qt::CheckStateRole && index.column() == 0 )
	{
		return m_checkStates.value( QIdentityProxyModel::data( index, m_uidRole ).toUuid() );
	}

	return QIdentityProxyModel::data(index, role);
}



bool CheckableItemProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if( role != Qt::CheckStateRole || index.column() > 0 )
	{
		return QIdentityProxyModel::setData( index, value, role );
	}

	if( m_callDepth > 1 )
	{
		return false;
	}

	++m_callDepth;

	const auto checkState = checkStateFromVariant( value );

	m_checkStates[QIdentityProxyModel::data( index, m_uidRole ).toUuid()] = checkState;

	const auto parentIndex = index.parent();

	int childrenCount = rowCount( index );

	if( childrenCount > 0 )
	{
		for( int i = 0; i < childrenCount; ++i )
		{
			setData( this->index( i, 0, index ), value, role );
		}

		emit dataChanged( this->index( 0, 0, parentIndex ), this->index( childrenCount-1, 0, parentIndex ), { role } );
	}
	else if( parentIndex.isValid() )
	{
		childrenCount = rowCount( parentIndex );

		Qt::CheckState parentCheckState = checkState;

		for( int i = 0; i < childrenCount; ++i )
		{
			if( checkStateFromVariant( data( this->index( i, 0, parentIndex ), role ) ) != parentCheckState )
			{
				parentCheckState = Qt::PartiallyChecked;
				break;
			}
		}

		if( checkStateFromVariant( data( parentIndex, Qt::CheckStateRole ) ) != parentCheckState )
		{
			setData( parentIndex, parentCheckState, role );
			if( m_callDepth == 0 )
			{
				emit dataChanged( parentIndex, parentIndex, { role } );
			}
		}

		emit dataChanged( index, index, { role } );
	}

	--m_callDepth;

	return true;
}



void CheckableItemProxyModel::updateNewRows(const QModelIndex &parent, int first, int last)
{
	// also set newly inserted items checked if parent is checked
	if( parent.isValid() && checkStateFromVariant( data( parent, Qt::CheckStateRole ) ) == Qt::Checked )
	{
		for( int i = first; i <= last; ++i )
		{
			setData( index( i, 0, parent ), Qt::Checked, Qt::CheckStateRole );
		}
	}
}



void CheckableItemProxyModel::removeRowStates(const QModelIndex &parent, int first, int last)
{
	for( int i = first; i <= last; ++i )
	{
		m_checkStates.remove( QIdentityProxyModel::data( index( i, 0, parent ), m_uidRole ).toUuid() );
	}
}



QJsonArray CheckableItemProxyModel::saveStates()
{
	QJsonArray data;

	for( auto it = m_checkStates.constBegin(), end = m_checkStates.constEnd(); it != end; ++it )
	{
		if( it.value() == Qt::Checked )
		{
			data += it.key().toString();
		}
	}

	return data;
}



void CheckableItemProxyModel::loadStates( const QJsonArray& data )
{
	beginResetModel();

	m_checkStates.clear();

	for( const auto& item : data )
	{
		const QUuid uid = QUuid( item.toString() );
		const auto indexList = match( index( 0, 0 ), m_uidRole, uid, 1,
									  Qt::MatchExactly | Qt::MatchRecursive );
		if( indexList.isEmpty() == false &&
				hasChildren( indexList.first() ) == false )
		{
			setData( indexList.first(), Qt::Checked, Qt::CheckStateRole );
		}
	}

	endResetModel();
}
