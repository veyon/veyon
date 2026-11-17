// SPDX-FileCopyrightText: 2016-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ConfigurationPage.h"

namespace Ui {
class GeneralConfigurationPage;
}

class GeneralConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	GeneralConfigurationPage();
	~GeneralConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private:
	void testAuthentication();
	bool testLogonAuthentication();
	bool testKeyFileAuthentication();

	void openLogFileDirectory();
	void updateLogFileLocations();
	void clearLogFiles();
	static QStringList logFileDirectories();

	void populateNetworkObjectDirectories();

	// login name for PlatformFilesystemFunctions::userLogFileDirectory() matching all users
	static constexpr auto AllUsersPattern = QLatin1String("*");

	static QString authenticationTestTitle()
	{
		return tr("Authentication test");
	}

	Ui::GeneralConfigurationPage* ui;
};
