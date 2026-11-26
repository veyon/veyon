// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ConfigurationPage.h"
#include "WindowsPlatformConfiguration.h"

namespace Ui {
class WindowsPlatformConfigurationPage;
}

class WindowsPlatformConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	WindowsPlatformConfigurationPage();
	~WindowsPlatformConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private:
	void validateLogonSwitchUserKeySequence();

	Ui::WindowsPlatformConfigurationPage* ui;
	WindowsPlatformConfiguration m_configuration;
};
