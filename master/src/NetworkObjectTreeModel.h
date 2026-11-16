// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "NetworkObjectModel.h"

class NetworkObjectDirectory;

class NetworkObjectTreeModel : public NetworkObjectModel
{
	Q_OBJECT
public:
	explicit NetworkObjectTreeModel(NetworkObjectDirectory* directory, QObject* parent = nullptr);

	QModelIndex index(int row, int column,
					  const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& index) const override;

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	bool hasChildren(const QModelIndex& parent) const override;
	bool canFetchMore(const QModelIndex& parent) const override;
	void fetchMore(const QModelIndex& parent) override;


private:
	void beginInsertObjects(NetworkObject::ModelId parentId, int index, int count);
	void endInsertObjects();

	void beginRemoveObjects(NetworkObject::ModelId parentId, int index, int count);
	void endRemoveObjects();

	void updateObject(NetworkObject::ModelId parent, int index);

	QModelIndex objectIndex(NetworkObject::ModelId object, int column = 0) const;
	const NetworkObject& object(const QModelIndex& index) const;

	NetworkObjectDirectory* m_directory;
};
