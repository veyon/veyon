/*
 * CheckedNetworkObjectListModel.cpp - data model returning all checked network objects
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

#include <QColor>

#include "CheckedNetworkObjectListModel.h"


CheckedNetworkObjectListModel::CheckedNetworkObjectListModel(QAbstractItemModel* sourceModel, QObject *parent) :
	QAbstractListModel(parent),
	m_sourceModel( sourceModel )
{
	connect( m_sourceModel, &QAbstractItemModel::dataChanged,
			 this, &CheckedNetworkObjectListModel::dataChanged );
	connect( m_sourceModel, &QAbstractItemModel::layoutChanged,
			 this, &CheckedNetworkObjectListModel::layoutChanged );
	connect( m_sourceModel, &QAbstractItemModel::modelReset,
			 this, &CheckedNetworkObjectListModel::modelReset );
}



int CheckedNetworkObjectListModel::rowCount(const QModelIndex &parent) const
{
	if( parent.isValid() )
	{
		return 0;
	}

	int count = 0;

	QModelIndex rootIndex = m_sourceModel->index( -1, -1 );

	int groupCount = m_sourceModel->rowCount( rootIndex );

	for( int i = 0; i < groupCount; ++i )
	{
		QModelIndex groupIndex = m_sourceModel->index( i, 0, rootIndex );
		int objectCount = m_sourceModel->rowCount( groupIndex );

		for( int j = 0; j < objectCount; ++j )
		{
			QModelIndex objectIndex = m_sourceModel->index( j, 0, groupIndex );
			if( m_sourceModel->data( objectIndex, Qt::CheckStateRole ).value<Qt::CheckState>() == Qt::Checked )
			{
				++count;
			}
		}
	}

	return count;
}



QVariant CheckedNetworkObjectListModel::data(const QModelIndex &index, int role) const
{
	if( index.isValid() == false )
	{
		return QVariant();
	}

	if( role == Qt::DecorationRole )
	{
		return QColor(Qt::darkBlue);
	}

	if( role != Qt::DisplayRole)
	{
		return QVariant();
	}

	int count = 0;

	QModelIndex rootIndex = m_sourceModel->index( -1, -1 );

	int groupCount = m_sourceModel->rowCount( rootIndex );

	for( int i = 0; i < groupCount; ++i )
	{
		QModelIndex groupIndex = m_sourceModel->index( i, 0, rootIndex );
		int objectCount = m_sourceModel->rowCount( groupIndex );

		for( int j = 0; j < objectCount; ++j )
		{
			QModelIndex objectIndex = m_sourceModel->index( j, 0, groupIndex );
			if( m_sourceModel->data( objectIndex, Qt::CheckStateRole ).value<Qt::CheckState>() == Qt::Checked )
			{
				if( count == index.row() )
				{
					return m_sourceModel->data( objectIndex, role );
				}

				++count;
			}
		}
	}

	return QVariant();
}
