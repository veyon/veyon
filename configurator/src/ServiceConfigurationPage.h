// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMap>

#include "ConfigurationPage.h"
#include "Plugin.h"

namespace Ui {
class ServiceConfigurationPage;
}

class VncServerPluginInterface;

class ServiceConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	ServiceConfigurationPage();
	~ServiceConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

public Q_SLOTS:
	void startService();
	void stopService();


private Q_SLOTS:
	void updateServiceControl();
	void updateVncServerPluginConfigurationWidget();

private:
	void populateVncServerPluginComboBox();

	Ui::ServiceConfigurationPage* ui;

	QMap<Plugin::Uid, VncServerPluginInterface*> m_vncServerPluginInterfaces;
	QWidget* m_vncServerPluginConfigurationWidget;
};
