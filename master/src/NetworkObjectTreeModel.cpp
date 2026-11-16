// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NetworkObjectDirectory.h"
#include "NetworkObjectTreeModel.h"

#if defined(QT_TESTLIB_LIB)
#include <QAbstractItemModelTester>
#endif


NetworkObjectTreeModel::NetworkObjectTreeModel(NetworkObjectDirectory* directory, QObject* parent) :
	NetworkObjectModel(parent),
	m_directory(directory)
{
#if defined(QT_TESTLIB_LIB)
	new QAbstractItemModelTester(this, QAbstractItemModelTester::FailureReportingMode::Warning, this);
#endif

	connect(m_directory, &NetworkObjectDirectory::objectsAboutToBeInserted,
			this, &NetworkObjectTreeModel::beginInsertObjects);
	connect(m_directory, &NetworkObjectDirectory::objectsInserted,
			this, &NetworkObjectTreeModel::endInsertObjects);

	connect(m_directory, &NetworkObjectDirectory::objectsAboutToBeRemoved,
			this, &NetworkObjectTreeModel::beginRemoveObjects);
	connect(m_directory, &NetworkObjectDirectory::objectsRemoved,
			this, &NetworkObjectTreeModel::endRemoveObjects);

	connect(m_directory, &NetworkObjectDirectory::objectChanged,
			this, &NetworkObjectTreeModel::updateObject);
}



QModelIndex NetworkObjectTreeModel::index(int row, int column, const QModelIndex& parent) const
{
	if (row < 0 || column < 0)
	{
		return {};
	}

	return createIndex(row, column, m_directory->childId(parent.internalId(), row));
}



QModelIndex NetworkObjectTreeModel::parent(const QModelIndex& index) const
{
	if (index.isValid() == false || index.internalId() == m_directory->rootId())
	{
		return {};
	}

	auto parentId = m_directory->parentId(index.internalId());
	if (parentId)
	{
		return objectIndex(parentId, index.column());
	}

	return {};
}



int NetworkObjectTreeModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid() == false)
	{
		return m_directory->childCount(m_directory->rootId());
	}

	return m_directory->childCount(parent.internalId());
}



int NetworkObjectTreeModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)

	return 1;
}



QVariant NetworkObjectTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		return tr("Locations/Computers");
	}

	return {};
}



QVariant NetworkObjectTreeModel::data(const QModelIndex& index, int role) const
{
	if (index.isValid() == false || index.internalId() == m_directory->rootId())
	{
		return {};
	}

	const auto& networkObject = object(index);

	if (networkObject.isValid() == false)
	{
		return {};
	}

	switch (role)
	{
	case UidRole: return networkObject.uid();
	case NameRole: return networkObject.name();
	case TypeRole: return QVariant::fromValue(networkObject.type());
	case HostAddressRole: return networkObject.hostAddress();
	case MacAddressRole: return networkObject.macAddress();
	case DirectoryAddressRole: return networkObject.directoryAddress();
	default: break;
	}

	return {};
}



bool NetworkObjectTreeModel::hasChildren(const QModelIndex& parent) const
{
	if (parent.isValid() == false)
	{
		return NetworkObjectModel::hasChildren(parent);
	}

	const auto& networkObject = object(parent);

	switch (networkObject.type())
	{
	case NetworkObject::Type::None:
	case NetworkObject::Type::Host:
	case NetworkObject::Type::Label:
		return false;
	default:
		break;
	}

	if (m_directory->childCount(networkObject.modelId()) > 0)
	{
		return true;
	}

	if (networkObject.isPopulated())
	{
		return false;
	}

	return true;
}



bool NetworkObjectTreeModel::canFetchMore(const QModelIndex& parent) const
{
	if (parent.isValid())
	{
		return object(parent).isPopulated() == false;
	}

	return false;
}



void NetworkObjectTreeModel::fetchMore(const QModelIndex& parent)
{
	m_directory->fetchObjects(object(parent));
}



void NetworkObjectTreeModel::beginInsertObjects(NetworkObject::ModelId parentId, int index, int count)
{
	beginInsertRows(objectIndex(parentId), index, index + count - 1);
}



void NetworkObjectTreeModel::endInsertObjects()
{
	endInsertRows();
}



void NetworkObjectTreeModel::beginRemoveObjects(NetworkObject::ModelId parentId, int index, int count)
{
	beginRemoveRows(objectIndex(parentId), index, index + count - 1);
}



void NetworkObjectTreeModel::endRemoveObjects()
{
	endRemoveRows();
}



void NetworkObjectTreeModel::updateObject(NetworkObject::ModelId parentId, int row)
{
	const auto index = createIndex(row, 0, parentId);

	Q_EMIT dataChanged(index, index);
}



QModelIndex NetworkObjectTreeModel::objectIndex(NetworkObject::ModelId object, int column) const
{
	if (object == m_directory->rootId())
	{
		return {};
	}

	const auto parentId = m_directory->parentId(object);
	const auto parentRow = m_directory->index(parentId, object);

	if (parentRow >= 0)
	{
		return createIndex(parentRow, column, object);
	}

	return {};
}



const NetworkObject& NetworkObjectTreeModel::object(const QModelIndex& index) const
{
	return m_directory->object(index.parent().internalId(), index.internalId());
}
