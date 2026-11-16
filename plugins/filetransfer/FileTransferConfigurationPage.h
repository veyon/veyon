// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ConfigurationPage.h"

namespace Ui {
class FileTransferConfigurationPage;
}

class FileTransferConfiguration;

class FileTransferConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	explicit FileTransferConfigurationPage(FileTransferConfiguration& configuration, QWidget* parent = nullptr);
	~FileTransferConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private:
	void browseDefaultSourceDirectory();
	void browseDestinationDirectory();
	void browseFilesToCollectSourceDirectory();
	void browseCollectedFilesDestinationDirectory();

	Ui::FileTransferConfigurationPage* ui;

	FileTransferConfiguration& m_configuration;
};
