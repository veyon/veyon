/*
 * ComputerListModel.cpp - data model for computer objects
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

#include "ComputerListModel.h"
#include "ComputerManager.h"


ComputerListModel::ComputerListModel(ComputerManager* manager, QObject *parent) :
	QAbstractListModel(parent),
	m_manager( manager )
{
	connect( m_manager, &ComputerManager::computerAboutToBeInserted,
			 this, &ComputerListModel::beginInsertComputer );
	connect( m_manager, &ComputerManager::computerInserted,
			 this, &ComputerListModel::endInsertComputer );

	connect( m_manager, &ComputerManager::computerAboutToBeRemoved,
			 this, &ComputerListModel::beginRemoveComputer );
	connect( m_manager, &ComputerManager::computerRemoved,
			 this, &ComputerListModel::endRemoveComputer );

	connect( m_manager, &ComputerManager::computerListAboutToBeReset,
			 this, &ComputerListModel::beginResetModel );
	connect( m_manager, &ComputerManager::computerListAboutToBeReset,
			 this, &ComputerListModel::endResetModel );

}



int ComputerListModel::rowCount(const QModelIndex &parent) const
{
	if( parent.isValid() )
	{
		return 0;
	}

	return m_manager->computerList().count();
}



QVariant ComputerListModel::data(const QModelIndex &index, int role) const
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

	return m_manager->computerList()[index.row()].name();
}



void ComputerListModel::beginInsertComputer(int index)
{
	beginInsertRows( QModelIndex(), index, index );
}



void ComputerListModel::endInsertComputer()
{
	endInsertRows();
}



void ComputerListModel::beginRemoveComputer(int index)
{
	beginRemoveRows( QModelIndex(), index, index );
}



void ComputerListModel::endRemoveComputer()
{
	endRemoveRows();
}



void ComputerListModel::reload()
{
	beginResetModel();
	endResetModel();
}
