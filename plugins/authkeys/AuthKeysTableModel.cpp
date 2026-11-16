// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AuthKeysManager.h"
#include "AuthKeysTableModel.h"

AuthKeysTableModel::AuthKeysTableModel(QObject* parent) :
	QAbstractTableModel(parent),
	m_manager(new AuthKeysManager(this)),
	m_keys()
{
}



AuthKeysTableModel::~AuthKeysTableModel()
{
	delete m_manager;
}



void AuthKeysTableModel::reload()
{
	beginResetModel();

	m_keys = m_manager->listKeys();

	endResetModel();
}



int AuthKeysTableModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)

	return ColumnCount;
}



int AuthKeysTableModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)

	return m_keys.size();
}



QVariant AuthKeysTableModel::data(const QModelIndex& index, int role) const
{
	if (index.isValid() == false || role != Qt::DisplayRole)
	{
		return {};
	}

	const auto& key = m_keys[index.row()];

	switch (index.column())
	{
	case ColumnKeyName: return key.split(QLatin1Char('/')).value(0);
	case ColumnKeyType: return key.split(QLatin1Char('/')).value(1);
	case ColumnAccessGroup: return m_manager->accessGroup(key);
	case ColumnKeyPairID: return m_manager->keyPairId(key);
	default: break;
	}

	return {};
}



QVariant AuthKeysTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
	{
		return {};
	}

	switch (section)
	{
	case ColumnKeyName: return tr("Name");
	case ColumnKeyType: return tr("Type");
	case ColumnAccessGroup: return tr("Access group");
	case ColumnKeyPairID: return tr("Pair ID");
	default:
		break;
	}

	return {};
}
