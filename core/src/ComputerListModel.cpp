/*
 * ComputerListModel.cpp - data model base class for computer objects
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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
#include "VeyonConfiguration.h"


ComputerListModel::ComputerListModel( QObject* parent ) :
	QAbstractListModel( parent ),
	m_displayRoleContent( VeyonCore::config().computerDisplayRoleContent() ),
	m_sortOrder( VeyonCore::config().computerMonitoringSortOrder() ),
	m_aspectRatio( VeyonCore::config().computerMonitoringAspectRatio() )
{
}



Qt::ItemFlags ComputerListModel::flags( const QModelIndex& index ) const
{
	auto defaultFlags = QAbstractListModel::flags( index );

	if( index.isValid() )
	{
		 return Qt::ItemIsDragEnabled | defaultFlags;
	}

	return Qt::ItemIsDropEnabled | defaultFlags;
}



Qt::DropActions ComputerListModel::supportedDragActions() const
{
	return Qt::MoveAction;
}



Qt::DropActions ComputerListModel::supportedDropActions() const
{
	return Qt::MoveAction;
}
