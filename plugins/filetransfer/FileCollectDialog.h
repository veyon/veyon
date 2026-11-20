// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

namespace Ui {
class FileCollectDialog;
}

class FileCollectController;
class FileCollectTreeModel;

class FileCollectDialog : public QDialog
{
	Q_OBJECT
public:
	explicit FileCollectDialog(FileCollectController* controller, QWidget* parent = nullptr);
	~FileCollectDialog() override;

private:
	void start();
	void showFailedFiles();
	void openOutputDirectory();

	void accept() override;
	void reject() override;

	Ui::FileCollectDialog* ui;

	FileCollectController* m_controller;
	FileCollectTreeModel* m_model;
};
