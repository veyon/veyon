// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ConfigurationPage.h"

namespace Ui {
class DemoConfigurationPage;
}

class DemoConfiguration;

class DemoConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	explicit DemoConfigurationPage(DemoConfiguration& configuration, QWidget* parent = nullptr);
	~DemoConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;


private:
	Ui::DemoConfigurationPage* ui;

	DemoConfiguration& m_configuration;
};
