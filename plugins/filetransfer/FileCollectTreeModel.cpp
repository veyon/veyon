// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#if defined(QT_TESTLIB_LIB)
#include <QAbstractItemModelTester>
#endif

#include "FileCollectController.h"
#include "FileCollectTreeModel.h"


FileCollectTreeModel::FileCollectTreeModel(FileCollectController* controller, QObject* parent) :
	QAbstractItemModel(parent),
	m_controller(controller),
	m_scheduledPixmap(QIcon(QStringLiteral(":/filetransfer/file-scheduled.png"))),
	m_transferringPixmap(QIcon(QStringLiteral(":/filetransfer/file-transferring.png"))),
	m_waitingPixmap(QIcon(QStringLiteral(":/filetransfer/file-waiting.png"))),
	m_finishedPixmap(QIcon(QStringLiteral(":/filetransfer/file-finished.png"))),
	m_failedPixmap(QIcon(QStringLiteral(":/core/toast-warning.png")))
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

	return int(Column::ColumnCount);
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



QVariant FileCollectTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal &&
		role == Qt::DisplayRole)
	{
		switch (Column(section))
		{
		case Column::Name: return tr("Name");
		case Column::FileCount: return tr("Number of files");
		case Column::Progress: return tr("Progress");
		default: break;
		}
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
	switch (Column(index.column()))
	{
	case Column::Name:
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
			case FileCollection::State::Finished:
				return collection.failedFiles.isEmpty() ? m_finishedPixmap : m_failedPixmap;
			case FileCollection::State::Cancelled: return m_failedPixmap;
			default: break;
			}
			break;

		case Qt::ToolTipRole:
			if (collection.isCancelled())
			{
				return collection.cancelReason;
			}
			if (collection.failedFiles.isEmpty() == false)
			{
				return collection.failedFiles.size() == 1 ?
					tr("1 file could not be collected.") :
					tr("%1 files could not be collected.").arg(collection.failedFiles.size());
			}
			break;
		}
		break;

	case Column::FileCount:
		// the number of files of a collection that was cancelled before the computer listed them is unknown
		if (role == Qt::DisplayRole && collection.state != FileCollection::State::Initializing &&
			(collection.isCancelled() == false || collection.files.isEmpty() == false))
		{
			const auto notCollectedFilesCount = collection.notCollectedFilesCount();
			if (notCollectedFilesCount == 0)
			{
				return collection.files.size();
			}
			return tr("%1 (%2 not collected)").arg(collection.files.size()).arg(notCollectedFilesCount);
		}
		return {};

	case Column::Progress:
		return role == Qt::DisplayRole ? collection.progress() : QVariant{};

	case Column::ColumnCount:
		break;
	}

	return {};
}



QVariant FileCollectTreeModel::fileData(const FileCollection& collection, const QModelIndex& index, int role) const
{
	// show the reason in every column of a file that was not collected, including the progress bar
	if (role == Qt::ToolTipRole && collection.isNotCollectedFile(index.row()))
	{
		return collection.notCollectedReason(index.row());
	}

	switch (Column(index.column()))
	{
	case Column::Name:
		switch (role)
		{
		case Qt::DecorationRole:
			if (collection.isNotCollectedFile(index.row()))
			{
				return m_failedPixmap;
			}
			if (index.row() < collection.processedFilesCount)
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
		default:
			break;
		}
		break;

	case Column::Progress:
		if (role == Qt::DisplayRole)
		{
			if (collection.isNotCollectedFile(index.row()))
			{
				return 0;
			}
			if (index.row() < collection.processedFilesCount)
			{
				return 100;
			}
			else if (index.row() > collection.processedFilesCount)
			{
				return 0;
			}

			return collection.currentFileProgress();
		}
		break;

	default:
		break;
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
				for (int column = 0; column < int(Column::ColumnCount); ++column)
				{
					Q_EMIT dataChanged(createIndex(0, column, &collection.files.first()),
									   createIndex(collection.files.count() - 1, column, &collection.files.last()));
				}
			}
		}
	}

	Q_EMIT dataChanged(indexOfCollection(collectionId, 0), indexOfCollection(collectionId, int(Column::ColumnCount) - 1));
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
