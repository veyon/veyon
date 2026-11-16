// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QMessageBox>
#include <QPushButton>
#include <QScreen>

#include "FileTransferController.h"
#include "FileTransferDialog.h"
#include "FileTransferListModel.h"
#include "Filesystem.h"

#include "ui_FileTransferDialog.h"

FileTransferDialog::FileTransferDialog(FileTransferController* controller, QWidget* parent) :
	QDialog(parent),
	ui(new Ui::FileTransferDialog),
	m_controller(controller),
	m_listModel(new FileTransferListModel(m_controller, this))
{
	ui->setupUi(this);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Start"));

	ui->fileListView->setModel(m_listModel);

	connect(m_controller, &FileTransferController::progressChanged,
			this, &FileTransferDialog::updateProgress);

	connect(m_controller, &FileTransferController::finished,
			this, &FileTransferDialog::finish);

	connect(m_controller, &FileTransferController::errorOccurred,
			this, &FileTransferDialog::reportError);

	const auto availableSize = screen()->availableSize();
	move(availableSize.width() / 8, availableSize.height() / 8);
	resize(availableSize * 3 / 4);
}



FileTransferDialog::~FileTransferDialog()
{
	delete ui;

	delete m_listModel;
}



void FileTransferDialog::accept()
{
	ui->optionsGroupBox->setDisabled(true);
	ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

	FileTransferController::Flags flags(FileTransferController::Transfer);

	if (ui->transferAndOpenFolder->isChecked())
	{
		flags |= FileTransferController::OpenTransferFolder;
	}

	if (ui->transferAndOpenProgram->isChecked())
	{
		flags |= FileTransferController::OpenFilesInApplication;
	}

	if (ui->overwriteExistingFiles->isChecked())
	{
		flags |= FileTransferController::OverwriteExistingFiles;
	}

	// Set custom destination directory if provided
	const auto destDir = ui->destinationDirectoryEdit->text().trimmed();
	if (destDir.isEmpty() == false)
	{
		m_controller->setDestinationDirectory(destDir);
	}

	m_controller->setFlags(flags);
	m_controller->start();
}



void FileTransferDialog::reject()
{
	if (m_controller->isRunning())
	{
		m_controller->stop();
	}

	QDialog::reject();
}



void FileTransferDialog::finish()
{
	ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
	connect(ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked,
			this, &FileTransferDialog::close);
}



void FileTransferDialog::updateProgress(int progress)
{
	ui->progressBar->setValue(progress);
}



void FileTransferDialog::reportError(const QString& message)
{
	QMessageBox::critical(this, tr("File transfer error"), message);
}
