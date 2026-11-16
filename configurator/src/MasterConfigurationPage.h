// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ConfigurationPage.h"

namespace Ui {
class MasterConfigurationPage;
}

class MasterConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	MasterConfigurationPage();
	~MasterConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private Q_SLOTS:
	void enableFeature();
	void disableFeature();

private:
	void openUserConfigurationDirectory();
	void openScreenshotDirectory();
	void openConfigurationTemplatesDirectory();
	void populateFeatureComboBox();
	void updateFeatureLists();

	Ui::MasterConfigurationPage* ui;

	QStringList m_disabledFeatures;
};
