/*
 * FileCollectTreeModel.cpp - implementation of FileCollectTreeModel class
 *
 * Copyright (c) 2025-2026 Tobias Junghans <tobydox@veyon.io>
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

#if defined(QT_TESTLIB_LIB)
#include <QAbstractItemModelTester>
#endif

#include "FileCollectTreeModel.h"
#include "FileCollectController.h"


FileCollectTreeModel::FileCollectTreeModel( FileCollectController* controller, QObject* parent ) :
	QAbstractItemModel(parent),
	m_controller(controller),
	m_scheduledPixmap(QIcon(QStringLiteral(":/filetransfer/file-scheduled.png"))),
	m_transferringPixmap(QIcon(QStringLiteral(":/filetransfer/file-transferring.png"))),
	m_waitingPixmap(QIcon(QStringLiteral(":/filetransfer/file-waiting.png"))),
	m_finishedPixmap(QIcon(QStringLiteral(":/filetransfer/file-finished.png")))
{
	connect(m_controller, &FileCollectController::collectionsAboutToChange,
			this, &FileCollectTreeModel::beginResetModel);
	connect(m_controller, &FileCollectController::collectionsChanged,
			this, &FileCollectTreeModel::endResetModel);

	connect(m_controller, &FileCollectController::collectionAboutToInitialize,
			this, &FileCollectTreeModel::beginInitializeCollection);
	connect(m_controller, &FileCollectController::collectionInitialized,
			this, &FileCollectTreeModel::endInitializeCollection);
	connect(m_controller, &FileCollectController::collectionChanged,
			this, &FileCollectTreeModel::updateCollection);

#if defined(QT_TESTLIB_LIB)
	new QAbstractItemModelTester(this, QAbstractItemModelTester::FailureReportingMode::Warning, this);
#endif
}



QModelIndex FileCollectTreeModel::index(int row, int column, const QModelIndex& parent) const
{
	if (parent.isValid())
	{
		const auto& collection = collectionAtRow(parent.row());
		if (isValidCollection(collection) && row < collection.files.count())
		{
			return createIndex(row, column, &collection.files[row]);
		}
		return {};
	}

	const auto& collection = collectionAtRow(row);
	if (isValidCollection(collection))
	{
		return createIndex(row, column, &collection);
	}
	return {};
}



QModelIndex FileCollectTreeModel::parent(const QModelIndex& index) const
{
	if (index.isValid() == false)
	{
		return {};
	}

	int collectionRow = 0;
	for (const auto& collection : m_controller->collections())
	{
		if (constInternalPointer(index) == &collection)
		{
			return {};
		}

		if (index.row() < collection.files.count() &&
			constInternalPointer(index) == &collection.files[index.row()])
		{
			return createIndex(collectionRow, index.column(), &collection);
		}

		++collectionRow;
	}

	return {};
}



int FileCollectTreeModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
	{
		for (const auto& collection : m_controller->collections())
		{
			if (constInternalPointer(parent) == &collection)
			{
				return collection.files.count();
			}
		}
		return 0;
	}

	return m_controller->collections().count();
}



int FileCollectTreeModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);

	return 2;
}



bool FileCollectTreeModel::hasChildren(const QModelIndex& parent) const
{
	if (parent.isValid())
	{
		for (const auto& collection : m_controller->collections())
		{
			if (constInternalPointer(parent) == &collection)
			{
				return true;
			}
		}
		return false;
	}

	return m_controller->collections().count() > 0;
}



QVariant FileCollectTreeModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
	{
		return {};
	}

	for (const auto& collection : m_controller->collections())
	{
		if (constInternalPointer(index) == &collection)
		{
			return collectionData(collection, index, role);
		}

		if (index.row() < collection.files.count() &&
			constInternalPointer(index) == &collection.files[index.row()])
		{
			return fileData(collection, index, role);
		}
	}
	return {};
}



QVariant FileCollectTreeModel::headerData(int section,  Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal &&
		role == Qt::DisplayRole)
	{
		if (section > 0)
		{
			return tr("Progress");
		}
		return tr("Name");
	}

	return {};
}



bool FileCollectTreeModel::isValidCollection(const FileCollection& collection) const
{
	return collection != m_dummyCollection;
}




QModelIndex FileCollectTreeModel::indexOfCollection(FileCollection::Id collectionId, int column) const
{
	int row = 0;

	for (const auto& collection : m_controller->collections())
	{
		if (collection.id == collectionId)
		{
			return createIndex(row, column, &collection);
		}
		++row;
	}

	return {};
}



QVariant FileCollectTreeModel::collectionData(const FileCollection& collection, const QModelIndex& index, int role) const
{
	if (index.column() > 0)
	{
		return role == Qt::DisplayRole ? collection.progress() : QVariant{};
	}

	switch (role)
	{
	case Qt::DisplayRole:
		return collection.name;

	case Qt::DecorationRole:
		switch (collection.state)
		{
		case FileCollection::State::Initializing: return m_scheduledPixmap;
		case FileCollection::State::ReadyForNextFileTransfer: return m_transferringPixmap;
		case FileCollection::State::FileTransferRunning: return m_transferringPixmap;
		case FileCollection::State::Finished: return m_finishedPixmap;
		default: break;
		}
		break;
	}

	return {};
}



QVariant FileCollectTreeModel::fileData(const FileCollection& collection, const QModelIndex& index, int role) const
{
	if (index.column() > 0)
	{
		if (role == Qt::DisplayRole)
		{
			if (index.row() < collection.processedFilesCount )
			{
				return 100;
			}
			else if (index.row() > collection.processedFilesCount)
			{
				return 0;
			}

			return collection.currentFileProgress();
		}
		return QVariant{};
	}

	switch (role)
	{
	case Qt::DecorationRole:
		if (index.row() < collection.processedFilesCount )
		{
			return m_finishedPixmap;
		}
		else if (index.row() > collection.processedFilesCount)
		{
			return m_scheduledPixmap;
		}
		else if (collection.currentFileProgress() <= 0)
		{
			return m_waitingPixmap;
		}
		return m_transferringPixmap;
	case Qt::DisplayRole:
		return collection.files.value(index.row());
	}

	return {};
}



void FileCollectTreeModel::beginInitializeCollection(FileCollection::Id fileCollection, int fileCount)
{
	beginInsertRows(indexOfCollection(fileCollection, 0), 0, fileCount - 1);
}



void FileCollectTreeModel::endInitializeCollection()
{
	endInsertRows();
}



void FileCollectTreeModel::updateCollection(FileCollection::Id collectionId)
{
	for (const auto& collection : m_controller->collections())
	{
		if (collection.id == collectionId)
		{
			if (collection.files.size() > 0)
			{
				for (int column = 0; column < 2; ++column)
				{
					Q_EMIT dataChanged(createIndex(0, column, &collection.files.first()),
									   createIndex(collection.files.count() - 1, column, &collection.files.last()));
				}
			}
		}
	}

	Q_EMIT dataChanged(indexOfCollection(collectionId, 0), indexOfCollection(collectionId, 1));
}



const FileCollection& FileCollectTreeModel::collectionAtRow(int row) const
{
	if (row >= 0 && row < m_controller->collections().count())
	{
		int currentRow = 0;

		for (const auto& collection : m_controller->collections())
		{
			if (row == currentRow)
			{
				return collection;
			}
			++currentRow;
		}
	}

	return m_dummyCollection;
}
