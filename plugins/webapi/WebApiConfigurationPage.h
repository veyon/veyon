// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ConfigurationPage.h"

namespace Ui {
class WebApiConfigurationPage;
}

class WebApiConfiguration;

class WebApiConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	explicit WebApiConfigurationPage(WebApiConfiguration& configuration, QWidget* parent = nullptr);
	~WebApiConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private:
	Ui::WebApiConfigurationPage* ui;

	WebApiConfiguration& m_configuration;
};
