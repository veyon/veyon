/*
 * NetworkObjectFilterProxyModel.cpp - implementation of NetworkObjectFilterProxyModel
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include "NetworkObjectModel.h"
#include "NetworkObjectFilterProxyModel.h"

#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
#include <QAbstractItemModelTester>
#endif


NetworkObjectFilterProxyModel::NetworkObjectFilterProxyModel( QObject* parent ) :
	QSortFilterProxyModel( parent )
{
#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	new QAbstractItemModelTester( this, QAbstractItemModelTester::FailureReportingMode::Warning, this );
#endif
}



void NetworkObjectFilterProxyModel::setGroupFilter( const QStringList& groupList )
{
	beginResetModel();
	m_groupList = groupList;
	endResetModel();
}



void NetworkObjectFilterProxyModel::setComputerExcludeFilter( const QStringList& computerExcludeList )
{
	beginResetModel();
	m_computerExcludeList = computerExcludeList;
	endResetModel();
}



void NetworkObjectFilterProxyModel::setEmptyGroupsExcluded(bool enabled)
{
	beginResetModel();
	m_excludeEmptyGroups = enabled;
	endResetModel();
}


void NetworkObjectFilterProxyModel::setComputersExcluded(bool enabled)
{
	beginResetModel();
	m_excludeComputers = enabled;
	endResetModel();
}



bool NetworkObjectFilterProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
	const auto index = sourceModel()->index(sourceRow, 0, sourceParent);
	const auto objectType = NetworkObject::Type(sourceModel()->data(index, NetworkObjectModel::TypeRole).toInt());
	if (filterAcceptsRowRecursive(index))
	{
		return true;
	}

	if (NetworkObject::isContainer(objectType))
	{
		return parentContainerAccepted(index);
	}

	return false;
}



bool NetworkObjectFilterProxyModel::filterAcceptsRowRecursive(const QModelIndex& index) const
{
	const auto objectType = NetworkObject::Type(sourceModel()->data(index, NetworkObjectModel::TypeRole).toInt());

	if (objectType == NetworkObject::Type::Host)
	{
		if (m_excludeComputers)
		{
			return false;
		}

		const auto parentAccepted = m_groupList.isEmpty() ||
									parentContainerAccepted(sourceModel()->parent(index));
		if (parentAccepted == false)
		{
			return false;
		}

		if( m_computerExcludeList.isEmpty() )
		{
			return true;
		}

		const auto hostAddress = sourceModel()->data(index, NetworkObjectModel::HostAddressRole).toString();

		return m_computerExcludeList.contains( hostAddress, Qt::CaseInsensitive ) == false;
	}
	else if (NetworkObject::isContainer(objectType))
	{
		if (sourceModel()->canFetchMore(index))
		{
			sourceModel()->fetchMore(index);
		}

		const auto rows = sourceModel()->rowCount(index);

		if (m_excludeEmptyGroups && rows == 0)
		{
			return false;
		}

		if (m_groupList.isEmpty())
		{
			return true;
		}

		for (int i = 0; i < rows; ++i)
		{
			const auto rowIndex = sourceModel()->index(i, 0, index);
			const auto objectType = NetworkObject::Type(sourceModel()->data(rowIndex, NetworkObjectModel::TypeRole).toInt());

			if (NetworkObject::isContainer(objectType) && filterAcceptsRowRecursive(rowIndex))
			{
				return true;
			}
		}

		return m_groupList.contains(sourceModel()->data(index).toString());
	}

	return true;
}



bool NetworkObjectFilterProxyModel::parentContainerAccepted(const QModelIndex& index) const
{
	return index.isValid() &&
			(m_groupList.contains(sourceModel()->data(index).toString()) ||
			 parentContainerAccepted(sourceModel()->parent(index)));
}
