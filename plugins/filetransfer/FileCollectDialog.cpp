/*
 * FileCollectDialog.cpp - implementation of FileCollectDialog
 *
 * Copyright (c) 2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QDesktopServices>
#include <QInputDialog>
#include <QPushButton>

#include "FileCollectController.h"
#include "FileCollectDialog.h"
#include "FileCollectTreeModel.h"
#include "Filesystem.h"
#include "ProgressItemDelegate.h"

#include "ui_FileCollectDialog.h"

FileCollectDialog::FileCollectDialog(FileCollectController* controller, QWidget* parent) :
	QDialog(parent),
	ui(new Ui::FileCollectDialog),
	m_controller(controller),
	m_model(new FileCollectTreeModel(controller, this))
{
	ui->setupUi(this);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Start"));

	ui->collectionsTreeView->setItemDelegateForColumn(1, new ProgressItemDelegate(ui->collectionsTreeView));
	ui->collectionsTreeView->setModel(m_model);

	connect (ui->openOutputDirectoryButton, &QAbstractButton::clicked, this, &FileCollectDialog::openOutputDirectory);

	connect (m_controller, &FileCollectController::overallProgressChanged, this, [this]() {
		ui->progressBar->setValue(m_controller->overallProgress());
	});
	connect (m_controller, &FileCollectController::started, this, [this]() {
		ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
	});
	connect (m_controller, &FileCollectController::finished, this, [this]() {
		ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
	});
}



FileCollectDialog::~FileCollectDialog()
{
	delete ui;
}



void FileCollectDialog::openOutputDirectory()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(ui->outputDirectoryEdit->text() + QDir::separator()));
}



void FileCollectDialog::accept()
{
	switch (m_controller->collectionDirectory())
	{
	case FileCollectController::CollectionDirectory::PromptUserForName:
	{
		const auto collectionName = QInputDialog::getText(this, tr("Enter collection name" ),
														  tr("Please enter a name for this file collection:"));
		if (collectionName.isEmpty())
		{
			return;
		}

		m_controller->setCollectionName(collectionName);
		break;
	}
	case FileCollectController::CollectionDirectory::DateTimeSubdirectory:
		m_controller->setCollectionName(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmm")));
		break;
	default:
		break;
	}

	if (VeyonCore::filesystem().ensurePathExists(m_controller->outputDirectory()) == false)
	{
		QMessageBox::critical(this, tr("Output directory creation failed"),
							  tr("The output directory \"%1\" does not exist and could not be created. "
								 "Please check the configuration and the file permissions for the configured destination directory."));
		return;
	}

	ui->outputDirectoryEdit->setText(m_controller->outputDirectory());
	ui->openOutputDirectoryButton->setEnabled(true);

	m_controller->start();
}



void FileCollectDialog::reject()
{
	if (m_controller->isRunning())
	{
		m_controller->stop();
		ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
	}
	else
	{
		QDialog::reject();
	}
}
