// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ConfigurationPage.h"
#include "LinuxPlatformConfiguration.h"

namespace Ui {
class LinuxPlatformConfigurationPage;
}

// clazy:excludeall=ctor-missing-parent-argument
class LinuxPlatformConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	LinuxPlatformConfigurationPage();
	~LinuxPlatformConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private:
	Ui::LinuxPlatformConfigurationPage* ui;
	LinuxPlatformConfiguration m_configuration;
};
