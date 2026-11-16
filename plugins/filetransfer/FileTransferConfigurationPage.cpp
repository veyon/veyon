// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/UiMapping.h"
#include "FileSystemBrowser.h"
#include "FileTransferConfiguration.h"
#include "FileTransferConfigurationPage.h"

#include "ui_FileTransferConfigurationPage.h"

FileTransferConfigurationPage::FileTransferConfigurationPage(FileTransferConfiguration& configuration, QWidget* parent) :
	ConfigurationPage(parent),
	ui(new Ui::FileTransferConfigurationPage),
	m_configuration(configuration)
{
	ui->setupUi(this);

	connect(ui->browseDefaultSourceDirectory, &QAbstractButton::clicked,
			this, &FileTransferConfigurationPage::browseDefaultSourceDirectory);

	connect(ui->browseDestinationDirectory, &QAbstractButton::clicked,
			this, &FileTransferConfigurationPage::browseDestinationDirectory);

	connect(ui->browseFilesToCollectSourceDirectory, &QAbstractButton::clicked,
			this, &FileTransferConfigurationPage::browseFilesToCollectSourceDirectory);

	connect(ui->browseCollectedFilesDestinationDirectory, &QAbstractButton::clicked,
			this, &FileTransferConfigurationPage::browseCollectedFilesDestinationDirectory);

	connect(ui->collectedFilesGroupingMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
		const auto groupingAttributesRequired = ui->collectedFilesGroupingMode->currentIndex() > 0;
		ui->collectedFilesGroupingAttribute1->setEnabled(groupingAttributesRequired);
		ui->collectedFilesGroupingAttribute2->setEnabled(groupingAttributesRequired);
	});

	ui->collectingMode->hide();
	ui->collectingModeLabel->hide();
}



FileTransferConfigurationPage::~FileTransferConfigurationPage()
{
	delete ui;
}



void FileTransferConfigurationPage::resetWidgets()
{
	FOREACH_FILE_TRANSFER_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void FileTransferConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_FILE_TRANSFER_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}



void FileTransferConfigurationPage::applyConfiguration()
{
}


void FileTransferConfigurationPage::browseDefaultSourceDirectory()
{
	FileSystemBrowser(FileSystemBrowser::ExistingDirectory, this).exec(ui->fileTransferDefaultSourceDirectory);
}



void FileTransferConfigurationPage::browseDestinationDirectory()
{
	FileSystemBrowser(FileSystemBrowser::ExistingDirectory, this).exec(ui->fileTransferDestinationDirectory);
}



void FileTransferConfigurationPage::browseFilesToCollectSourceDirectory()
{
	FileSystemBrowser(FileSystemBrowser::ExistingDirectory, this).exec(ui->filesToCollectSourceDirectory);
}



void FileTransferConfigurationPage::browseCollectedFilesDestinationDirectory()
{
	FileSystemBrowser(FileSystemBrowser::ExistingDirectory, this).exec(ui->collectedFilesDestinationDirectory);
}
