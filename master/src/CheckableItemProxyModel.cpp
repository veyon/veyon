/*
 * CheckableItemProxyModel.cpp - proxy model for overlaying checked property
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "CheckableItemProxyModel.h"
#include "QtCompat.h"

#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
#include <QAbstractItemModelTester>
#endif


CheckableItemProxyModel::CheckableItemProxyModel( int uidRole, QObject *parent ) :
	QIdentityProxyModel(parent),
	m_uidRole( uidRole ),
	m_exceptionRole( -1 ),
	m_exceptionData()
{
	connect( this, &QIdentityProxyModel::rowsInserted,
			 this, &CheckableItemProxyModel::updateNewRows );
	connect( this, &QIdentityProxyModel::rowsAboutToBeRemoved,
			 this, &CheckableItemProxyModel::removeRowStates );

#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	new QAbstractItemModelTester( this, QAbstractItemModelTester::FailureReportingMode::Warning, this );
#endif
}



void CheckableItemProxyModel::setException( int exceptionRole, const QVariant& exceptionData )
{
	beginResetModel();

	m_exceptionRole = exceptionRole;
	m_exceptionData = exceptionData;

	endResetModel();
}



Qt::ItemFlags CheckableItemProxyModel::flags( const QModelIndex& index ) const
{
	if( index.isValid() == false || index.column() > 0 ||
			( m_exceptionRole >= 0 && QIdentityProxyModel::data( index, m_exceptionRole ) == m_exceptionData ) )
	{
		return QIdentityProxyModel::flags( index );
	}

	return QIdentityProxyModel::flags( index ) | Qt::ItemIsUserCheckable;
}



QVariant CheckableItemProxyModel::data( const QModelIndex& index, int role ) const
{
	if( !index.isValid() )
	{
		return QVariant();
	}

	if( role == Qt::CheckStateRole && index.column() == 0 &&
			flags( index ).testFlag( Qt::ItemIsUserCheckable ) )
	{
		return m_checkStates.value( indexToUuid( index ) );
	}

	return QIdentityProxyModel::data(index, role);
}



bool CheckableItemProxyModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
	if( role != Qt::CheckStateRole || index.column() > 0 )
	{
		return QIdentityProxyModel::setData( index, value, role );
	}

	const auto uuid = indexToUuid( index );
	const auto checkState = checkStateFromVariant( value );

	if( qAsConst(m_checkStates)[uuid] != checkState )
	{
		m_checkStates[uuid] = checkState;
		emit dataChanged( index, index, { Qt::CheckStateRole } );

		setChildData( index, checkState );
		setParentData( index.parent(), checkState );
	}

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



QUuid CheckableItemProxyModel::indexToUuid( const QModelIndex& index ) const
{
	return QIdentityProxyModel::data( index, m_uidRole ).toUuid();
}



bool CheckableItemProxyModel::setChildData( const QModelIndex& index, Qt::CheckState checkState )
{
	bool modified = false;

	if( canFetchMore( index ) )
	{
		fetchMore( index );
	}

	const auto childCount = rowCount( index );

	if( childCount > 0 )
	{
		for( int i = 0; i < childCount; ++i )
		{
			const auto childIndex = this->index( i, 0, index );
			const auto childUuid = indexToUuid( childIndex );

			if( qAsConst(m_checkStates)[childUuid] != checkState )
			{
				m_checkStates[childUuid] = checkState;
				modified = true;
			}

			if( setChildData( childIndex, checkState ) )
			{
				modified = true;
			}
		}

		if( modified )
		{
			emit dataChanged( this->index( 0, 0, index ), this->index( childCount-1, 0, index ), { Qt::CheckStateRole } );
		}
	}

	return modified;
}



void CheckableItemProxyModel::setParentData( const QModelIndex& index, Qt::CheckState checkState )
{
	if( index.isValid() == false )
	{
		return;
	}

	const auto childCount = rowCount( index );

	for( int i = 0; i < childCount; ++i )
	{
		if( checkStateFromVariant( data( this->index( i, 0, index ), Qt::CheckStateRole ) ) != checkState )
		{
			checkState = Qt::PartiallyChecked;
			break;
		}
	}

	const auto uuid = indexToUuid( index );

	if( qAsConst(m_checkStates)[uuid] != checkState )
	{
		m_checkStates[uuid] = checkState;
		emit dataChanged( index, index, { Qt::CheckStateRole } );

		setParentData( index.parent(), checkState );
	}
}
