// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/UiMapping.h"
#include "DesktopServicesConfiguration.h"
#include "DesktopServicesConfigurationPage.h"
#include "ObjectManager.h"

#include "ui_DesktopServicesConfigurationPage.h"

DesktopServicesConfigurationPage::DesktopServicesConfigurationPage(DesktopServicesConfiguration& configuration, QWidget* parent) :
	ConfigurationPage(parent),
	ui(new Ui::DesktopServicesConfigurationPage),
	m_configuration(configuration)
{
	ui->setupUi(this);
}



DesktopServicesConfigurationPage::~DesktopServicesConfigurationPage()
{
	delete ui;
}



void DesktopServicesConfigurationPage::resetWidgets()
{
	loadObjects(m_configuration.predefinedApplications(), ui->applicationTable);
	loadObjects(m_configuration.predefinedWebsites(), ui->websiteTable);
}



void DesktopServicesConfigurationPage::connectWidgetsToProperties()
{
}



void DesktopServicesConfigurationPage::applyConfiguration()
{
}



void DesktopServicesConfigurationPage::addApplication()
{
	auto applications = m_configuration.predefinedApplications();

	addServiceObject(ui->applicationTable, DesktopServiceObject::Type::Application, tr("New application"), applications);

	m_configuration.setPredefinedApplications(applications);
}



void DesktopServicesConfigurationPage::updateApplication()
{
	auto applications = m_configuration.predefinedApplications();

	updateServiceObject(ui->applicationTable, DesktopServiceObject::Type::Application, applications);

	m_configuration.setPredefinedApplications(applications);
}



void DesktopServicesConfigurationPage::removeApplication()
{
	auto applications = m_configuration.predefinedApplications();

	removeServiceObject(ui->applicationTable, DesktopServiceObject::Type::Application, applications);

	m_configuration.setPredefinedApplications(applications);
}



void DesktopServicesConfigurationPage::addWebsite()
{
	auto websites = m_configuration.predefinedWebsites();

	addServiceObject(ui->websiteTable, DesktopServiceObject::Type::Website, tr("New website"), websites);

	m_configuration.setPredefinedWebsites(websites);
}

void DesktopServicesConfigurationPage::updateWebsite()
{
	auto websites = m_configuration.predefinedWebsites();

	updateServiceObject(ui->websiteTable, DesktopServiceObject::Type::Website, websites);

	m_configuration.setPredefinedWebsites(websites);
}



void DesktopServicesConfigurationPage::removeWebsite()
{
	auto websites = m_configuration.predefinedWebsites();

	removeServiceObject(ui->websiteTable, DesktopServiceObject::Type::Website, websites);

	m_configuration.setPredefinedWebsites(websites);
}



void DesktopServicesConfigurationPage::addServiceObject(QTableWidget* tableWidget, DesktopServiceObject::Type type,
														const QString& name, QJsonArray& objects)
{
	ObjectManager<DesktopServiceObject> objectManager(objects);
	objectManager.add(DesktopServiceObject(type, objectManager.generateUniqueName(name)));
	objects = objectManager.objects();

	loadObjects(objects, tableWidget);

	tableWidget->setCurrentCell(tableWidget->rowCount() - 1, 0);
}



void DesktopServicesConfigurationPage::updateServiceObject(QTableWidget* tableWidget, DesktopServiceObject::Type type, QJsonArray& objects)
{
	auto currentServiceObjectIndex = tableWidget->currentIndex();
	if (currentServiceObjectIndex.isValid() == false)
	{
		return;
	}

	ObjectManager<DesktopServiceObject> objectManager(objects);
	objectManager.update(currentServiceObject(tableWidget, type));
	objects = objectManager.objects();

	loadObjects(objects, tableWidget);

	tableWidget->setCurrentIndex(currentServiceObjectIndex);
}



void DesktopServicesConfigurationPage::removeServiceObject(QTableWidget* tableWidget, DesktopServiceObject::Type type, QJsonArray& objects)
{
	ObjectManager<DesktopServiceObject> objectManager(objects);
	objectManager.remove(currentServiceObject(tableWidget, type).uid());
	objects = objectManager.objects();

	loadObjects(objects, tableWidget);
}



DesktopServiceObject DesktopServicesConfigurationPage::currentServiceObject(QTableWidget* tableWidget, DesktopServiceObject::Type type)
{
	const auto row = tableWidget->currentRow();

	if (row >= 0)
	{
		auto nameItem = tableWidget->item(row, 0);
		auto pathItem = tableWidget->item(row, 1);

		return DesktopServiceObject(type,
									nameItem->text(),
									pathItem->text(),
									nameItem->data(Qt::UserRole).toUuid());
	}

	return DesktopServiceObject();
}



void DesktopServicesConfigurationPage::loadObjects(const QJsonArray& objects, QTableWidget* tableWidget)
{
	tableWidget->setUpdatesEnabled(false);
	tableWidget->setRowCount(0);

	int rowCount = 0;

	for (const auto& jsonValue : objects)
	{
		const auto serviceObject = DesktopServiceObject(jsonValue.toObject());

		auto item = new QTableWidgetItem(serviceObject.name());
		item->setData(Qt::UserRole, serviceObject.uid());

		tableWidget->setRowCount(rowCount + 1);
		tableWidget->setItem(rowCount, 0, item);
		tableWidget->setItem(rowCount, 1, new QTableWidgetItem(serviceObject.path()));
		++rowCount;
	}

	tableWidget->setUpdatesEnabled(true);
}
