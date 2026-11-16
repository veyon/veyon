// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QIcon>
#include <QStringListModel>

class FileTransferController;

class FileTransferListModel : public QStringListModel
{
	Q_OBJECT
public:
	FileTransferListModel(FileTransferController* controller, QObject* parent);
	~FileTransferListModel() override = default;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

private:
	FileTransferController* m_controller;
	QIcon m_scheduledPixmap;
	QIcon m_transferringPixmap;
	QIcon m_finishedPixmap;
};
