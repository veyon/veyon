// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ConfigurationPage.h"

namespace Ui {
class InternetBlockConfigurationPage;
}

class InternetBlockConfiguration;

class InternetBlockConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	explicit InternetBlockConfigurationPage(InternetBlockConfiguration& configuration, QWidget* parent = nullptr);
	~InternetBlockConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private:
	void validateAllowedNetworks();

	Ui::InternetBlockConfigurationPage* ui;

	InternetBlockConfiguration& m_configuration;
};
