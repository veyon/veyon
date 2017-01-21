/*
 * NetworkObjectModel.cpp - data model returning hierarchically grouped network objects
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

#include "NetworkObjectModel.h"

NetworkObjectModel::NetworkObjectModel(QObject *parent)
	: QAbstractItemModel(parent)
{
	m_groups << "Lab 1" << "Lab 2" << "Lab 3";

	m_objects["Lab 1"] += "Computer B";
	m_objects["Lab 1"] += "Computer C";
	m_objects["Lab 1"] += "Computer D";
	m_objects["Lab 1"] += "Computer A";
	m_objects["Lab 2"] += "Computer X";
	m_objects["Lab 2"] += "Computer Y";
	m_objects["Lab 2"] += "Computer Z";
}



QModelIndex NetworkObjectModel::index(int row, int column, const QModelIndex &parent) const
{
	if( parent.isValid() && parent.column() != 0 )
	{
		return QModelIndex();
	}

	quintptr parentId = 0;
	if( parent.isValid() && parent.internalId() == 0 )
	{
		parentId = parent.row() + 1;
	}

	return createIndex( row, column, parentId );
}



QModelIndex NetworkObjectModel::parent(const QModelIndex &index) const
{
	// parent ID set?
	if( index.internalId() > 0 )
	{
		// use it as row index
		return createIndex( index.internalId()-1, 0 );
	}

	return QModelIndex();
}



int NetworkObjectModel::rowCount(const QModelIndex &parent) const
{
	if( !parent.isValid() )
	{
		return m_groups.count();
	}

	if( parent.internalId() == 0 )
	{
		return m_objects[m_groups[parent.row()]].count();
	}

	return 0;
}



int NetworkObjectModel::columnCount(const QModelIndex &parent) const
{
	if( parent.isValid() == false || parent.internalId() == 0 )
	{
		return 1;
	}

	return 0;
}



QVariant NetworkObjectModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if( section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole )
	{
		return tr( "Name" );
	}

	return QVariant();
}



QVariant NetworkObjectModel::data(const QModelIndex &index, int role) const
{
	if( index.isValid() == false || role != Qt::DisplayRole )
	{
		return QVariant();
	}

	if( index.internalId() > 0 )
	{
		return m_objects.value( m_groups[index.internalId()-1] ).value( index.row() );
	}

	return m_groups.value( index.row() );
}
