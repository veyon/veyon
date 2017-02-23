/*
 * StringListFilterProxyModel.cpp - implementation of StringListFilterProxyModel
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "StringListFilterProxyModel.h"

StringListFilterProxyModel::StringListFilterProxyModel( QObject* parent ) :
	QSortFilterProxyModel( parent ),
	m_stringList()
{
}



void StringListFilterProxyModel::setStringList( const QStringList& stringList )
{
	beginResetModel();
	m_stringList = stringList;
	endResetModel();
}



bool StringListFilterProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
	Q_UNUSED(sourceParent);

	if( sourceParent.isValid() || m_stringList.isEmpty() )
	{
		return true;
	}

	return m_stringList.contains( sourceModel()->data( sourceModel()->index( sourceRow, 0 ) ).toString() );
}
