/*
 * NetworkObjectModelFactory.cpp - factor class for NetworkObjectModel instances
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

#include <QSortFilterProxyModel>

#include "NetworkObjectModelFactory.h"

#include "TestNetworkObjectDirectory.h"
#include "CheckableItemProxyModel.h"
#include "NetworkObjectTreeModel.h"


QAbstractItemModel* NetworkObjectModelFactory::create( QObject* parent )
{
	auto networkObjectDirectory = new TestNetworkObjectDirectory( parent );

	auto networkObjectTreeModel = new NetworkObjectTreeModel( networkObjectDirectory, parent );

	auto checkableItemProxy = new CheckableItemProxyModel( NetworkObjectTreeModel::NetworkObjectUidRole, parent );
	checkableItemProxy->setSourceModel( networkObjectTreeModel );

	QSortFilterProxyModel* sortProxy = new QSortFilterProxyModel( parent );
	sortProxy->setSourceModel( checkableItemProxy );

	return sortProxy;
}
