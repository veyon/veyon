/*
 * CheckableItemProxyModel.cpp - proxy model for overlaying checked property
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

CheckableItemProxyModel::CheckableItemProxyModel( int hashRole, QObject *parent ) :
	QIdentityProxyModel(parent),
	m_hashRole( hashRole ),
	m_callDepth( 0 )
{
	connect( this, &QIdentityProxyModel::rowsInserted,
			 this, &CheckableItemProxyModel::updateNewRows );
	connect( this, &QIdentityProxyModel::rowsAboutToBeRemoved,
			 this, &CheckableItemProxyModel::removeRowStates );
}



Qt::ItemFlags CheckableItemProxyModel::flags(const QModelIndex &index) const
{
	if( index.isValid() == false )
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

	if( role == Qt::CheckStateRole )
	{
		return m_checkStates.value( QIdentityProxyModel::data( index, m_hashRole ).toUInt() );
	}

	return QIdentityProxyModel::data(index, role);
}



bool CheckableItemProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if( role != Qt::CheckStateRole )
	{
		return QIdentityProxyModel::setData( index, value, role );
	}

	if( m_callDepth > 1 )
	{
		return false;
	}

	++m_callDepth;

	Qt::CheckState checkState = value.value<Qt::CheckState>();

	m_checkStates[QIdentityProxyModel::data( index, m_hashRole ).toUInt()] = checkState;

	int childrenCount = rowCount( index );

	if( childrenCount > 0 )
	{
		for( int i = 0; i < childrenCount; ++i )
		{
			setData( this->index( i, 0, index ), value, role );
		}

		emit dataChanged( this->index( 0, 0, index ), this->index( childrenCount, 0, index ), QVector<int>( role ) );
	}
	else if( index.parent().isValid() )
	{
		childrenCount = rowCount( index.parent() );

		Qt::CheckState parentCheckState = checkState;

		for( int i = 0; i < childrenCount; ++i )
		{
			if( data( this->index( i, 0, index.parent() ), role ).value<Qt::CheckState>() != parentCheckState )
			{
				parentCheckState = Qt::PartiallyChecked;
				break;
			}
		}

		setData( index.parent(), parentCheckState, role );

		emit dataChanged( index.parent(), index.parent(), QVector<int>( role ) );
	}

	--m_callDepth;

	return true;
}



void CheckableItemProxyModel::updateNewRows(const QModelIndex &parent, int first, int last)
{
	// also set newly inserted items checked if parent is checked
	if( parent.isValid() && data( parent, Qt::CheckStateRole ).value<Qt::CheckState>() == Qt::Checked )
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
		m_checkStates.remove( QIdentityProxyModel::data( index( i, 0, parent ), m_hashRole ).toUInt() );
	}
}
