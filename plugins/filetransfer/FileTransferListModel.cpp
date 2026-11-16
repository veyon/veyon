// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FileTransferController.h"
#include "FileTransferListModel.h"


FileTransferListModel::FileTransferListModel(FileTransferController* controller, QObject* parent) :
	QStringListModel(parent),
	m_controller(controller),
	m_scheduledPixmap(QIcon(QStringLiteral(":/filetransfer/file-scheduled.png"))),
	m_transferringPixmap(QIcon(QStringLiteral(":/filetransfer/file-transferring.png"))),
	m_finishedPixmap(QIcon(QStringLiteral(":/filetransfer/file-finished.png")))
{
	setStringList(m_controller->files());

	connect(m_controller, &FileTransferController::filesChanged,
			this, [this]() { setStringList(m_controller->files()); });

	connect(m_controller, &FileTransferController::progressChanged,
			this, [this]() { Q_EMIT dataChanged(index(0), index(rowCount()), {Qt::DecorationRole}); });

	connect(m_controller, &FileTransferController::started,
			this, [this]() { setStringList(m_controller->files()); });
}



QVariant FileTransferListModel::data(const QModelIndex& index, int role) const
{
	if (index.isValid() && role == Qt::DecorationRole)
	{
		int currentRow = m_controller->currentFileIndex();

		if (index.row() < currentRow)
		{
			return m_finishedPixmap;
		}
		else if (index.row() > currentRow || m_controller->isRunning() == false)
		{
			return m_scheduledPixmap;
		}

		return m_transferringPixmap;
	}

	return QStringListModel::data(index, role);
}
