// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

namespace Ui {
class FileTransferDialog;
}

class FileTransferController;
class FileTransferListModel;

class FileTransferDialog : public QDialog
{
	Q_OBJECT
public:
	FileTransferDialog(FileTransferController* controller, QWidget* parent);
	~FileTransferDialog() override;

private:
	void accept() override;
	void reject() override;
	void finish();

	void updateProgress(int progress);
	void reportError(const QString& message);

	Ui::FileTransferDialog* ui;

	FileTransferController* m_controller;
	FileTransferListModel* m_listModel;
};
