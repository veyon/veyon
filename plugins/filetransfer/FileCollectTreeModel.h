// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QAbstractItemModel>
#include <QIcon>

#include "FileCollection.h"

class FileCollectController;

class FileCollectTreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	enum class Column {
		Name,
		FileCount,
		Progress,
		ColumnCount
	};

	FileCollectTreeModel(FileCollectController* controller, QObject* parent);
	~FileCollectTreeModel() override = default;

	QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& index) const override;

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	bool hasChildren(const QModelIndex& parent) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	QVariant headerData(int section, Qt::Orientation orientation,
						int role) const override;

private:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QModelIndex createIndex(int row, int column, const void* data) const
	{
		return QAbstractItemModel::createIndex(row, column, quintptr(data));
	}
#endif
	static const void* constInternalPointer(const QModelIndex& index)
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		return index.constInternalPointer();
#else
		return reinterpret_cast<const void*>(index.internalId());
#endif
	}

	bool isValidCollection(const FileCollection& collection) const;

	QModelIndex indexOfCollection(FileCollection::Id collectionId, int column) const;

	QVariant collectionData(const FileCollection& collection, const QModelIndex& index, int role) const;
	QVariant fileData(const FileCollection& collection, const QModelIndex& index, int role) const;

	void beginInitializeCollection(FileCollection::Id collectionId, int fileCount);
	void endInitializeCollection();
	void updateCollection(FileCollection::Id collectionId);

	const FileCollection& collectionAtRow(int row) const;

	FileCollectController* m_controller;
	FileCollection m_dummyCollection{};
	QIcon m_scheduledPixmap;
	QIcon m_transferringPixmap;
	QIcon m_waitingPixmap;
	QIcon m_finishedPixmap;
};
