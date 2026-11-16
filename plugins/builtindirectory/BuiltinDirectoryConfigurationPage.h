// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ConfigurationPage.h"
#include "NetworkObject.h"

namespace Ui {
class BuiltinDirectoryConfigurationPage;
}

class BuiltinDirectoryConfiguration;

class BuiltinDirectoryConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	explicit BuiltinDirectoryConfigurationPage(BuiltinDirectoryConfiguration& configuration, QWidget* parent = nullptr);
	~BuiltinDirectoryConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private Q_SLOTS:
	void addLocation();
	void updateLocation();
	void removeLocation();
	void moveLocationUp();
	void moveLocationDown();
	void addComputer();
	void updateComputer();
	void removeComputer();
	void moveComputerUp();
	void moveComputerDown();

private:
	void populateLocations();
	void populateComputers();

	NetworkObject currentLocationObject() const;
	NetworkObject currentComputerObject() const;

	Ui::BuiltinDirectoryConfigurationPage* ui;

	BuiltinDirectoryConfiguration& m_configuration;
};
