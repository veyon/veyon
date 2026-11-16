// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NetworkObjectFilterProxyModel.h"
#include "NetworkObjectModel.h"

#if defined(QT_TESTLIB_LIB)
#include <QAbstractItemModelTester>
#endif


NetworkObjectFilterProxyModel::NetworkObjectFilterProxyModel(QObject* parent) :
	QSortFilterProxyModel(parent),
	m_groupList(),
	m_computerExcludeList(),
	m_excludeEmptyGroups(false)
{
#if defined(QT_TESTLIB_LIB)
	new QAbstractItemModelTester(this, QAbstractItemModelTester::FailureReportingMode::Warning, this);
#endif
}



void NetworkObjectFilterProxyModel::setGroupFilter(const QStringList& groupList)
{
	beginResetModel();
	m_groupList = groupList;
	endResetModel();
}



void NetworkObjectFilterProxyModel::setComputerExcludeFilter(const QStringList& computerExcludeList)
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



bool NetworkObjectFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	const auto index = sourceModel()->index(sourceRow, 0, sourceParent);
	if (filterAcceptsRowRecursive(index))
	{
		return true;
	}

	const auto objectType = NetworkObject::Type(sourceModel()->data(index, NetworkObjectModel::TypeRole).toInt());
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
			parentContainerAccepted(index.parent());
		if (parentAccepted == false)
		{
			return false;
		}

		if (m_computerExcludeList.isEmpty())
		{
			return true;
		}

		const auto hostAddress = sourceModel()->data(index, NetworkObjectModel::HostAddressRole).toString();

		return m_computerExcludeList.contains(hostAddress, Qt::CaseInsensitive) == false;
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
			const auto rowObjectType = NetworkObject::Type(sourceModel()->data(rowIndex, NetworkObjectModel::TypeRole).toInt());

			if (NetworkObject::isContainer(rowObjectType) && filterAcceptsRowRecursive(rowIndex))
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
		 parentContainerAccepted(index.parent()));
}
