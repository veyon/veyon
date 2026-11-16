// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ConfigurationPage.h"
#include "DesktopServiceObject.h"

class QTableWidget;
class DesktopServicesConfiguration;

namespace Ui {
class DesktopServicesConfigurationPage;
}

class DesktopServicesConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	explicit DesktopServicesConfigurationPage(DesktopServicesConfiguration& configuration, QWidget* parent = nullptr);
	~DesktopServicesConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private Q_SLOTS:
	void addApplication();
	void updateApplication();
	void removeApplication();
	void addWebsite();
	void updateWebsite();
	void removeWebsite();

private:
	static void addServiceObject(QTableWidget* tableWidget, DesktopServiceObject::Type type, const QString& name, QJsonArray& objects);
	static void updateServiceObject(QTableWidget* tableWidget, DesktopServiceObject::Type type, QJsonArray& objects);
	static void removeServiceObject(QTableWidget* tableWidget, DesktopServiceObject::Type type, QJsonArray& objects);

	static DesktopServiceObject currentServiceObject(QTableWidget* tableWidget, DesktopServiceObject::Type type);

	static void loadObjects(const QJsonArray& objects, QTableWidget* tableWidget);

	Ui::DesktopServicesConfigurationPage* ui;

	DesktopServicesConfiguration& m_configuration;
};
