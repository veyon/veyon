// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BuiltinFeatures.h"
#include "Configuration/UiMapping.h"
#include "FeatureManager.h"
#include "FileSystemBrowser.h"
#include "MasterConfigurationPage.h"
#include "MonitoringMode.h"
#include "PluginManager.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"

#include "ui_MasterConfigurationPage.h"


MasterConfigurationPage::MasterConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::MasterConfigurationPage)
{
	ui->setupUi(this);

	Configuration::UiMapping::setFlags(ui->advancedSettingsGroupBox, Configuration::Property::Flag::Advanced);

	connect(ui->openUserConfigurationDirectory, &QPushButton::clicked,
			this, &MasterConfigurationPage::openUserConfigurationDirectory);

	connect(ui->openScreenshotDirectory, &QPushButton::clicked,
			this, &MasterConfigurationPage::openScreenshotDirectory);

	connect(ui->openConfigurationTemplatesDirectory, &QPushButton::clicked,
			this, &MasterConfigurationPage::openConfigurationTemplatesDirectory);

	connect(ui->guestUserProperty, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
		ui->guestUserValue->setEnabled(ui->guestUserProperty->currentIndex() > 0);
	});

	connect(ui->guestUserIdentityExtensionType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
		ui->guestUserIdentityExtensionValue->setEnabled(ui->guestUserIdentityExtensionType->currentIndex() > 0);
	});


	populateFeatureComboBox();
}



MasterConfigurationPage::~MasterConfigurationPage()
{
	delete ui;
}



void MasterConfigurationPage::resetWidgets()
{
	FOREACH_VEYON_DIRECTORIES_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_VEYON_MASTER_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);

	m_disabledFeatures = VeyonCore::config().disabledFeatures();

	updateFeatureLists();
}



void MasterConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_VEYON_DIRECTORIES_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_VEYON_MASTER_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void MasterConfigurationPage::applyConfiguration()
{
}



void MasterConfigurationPage::enableFeature()
{
	const auto items = ui->disabledFeaturesListWidget->selectedItems();

	for (auto item : items)
	{
		m_disabledFeatures.removeAll(item->data(Qt::UserRole).toString());
	}

	VeyonCore::config().setDisabledFeatures(m_disabledFeatures);

	updateFeatureLists();
}



void MasterConfigurationPage::disableFeature()
{
	const auto items = ui->allFeaturesListWidget->selectedItems();

	for (auto item : items)
	{
		QString featureUid = item->data(Qt::UserRole).toString();
		m_disabledFeatures.removeAll(featureUid);
		m_disabledFeatures.append(featureUid);
	}

	VeyonCore::config().setDisabledFeatures(m_disabledFeatures);

	updateFeatureLists();
}



void MasterConfigurationPage::openUserConfigurationDirectory()
{
	FileSystemBrowser(FileSystemBrowser::ExistingDirectory, this).exec(ui->userConfigurationDirectory);
}



void MasterConfigurationPage::openScreenshotDirectory()
{
	FileSystemBrowser(FileSystemBrowser::ExistingDirectory, this).exec(ui->screenshotDirectory);
}



void MasterConfigurationPage::openConfigurationTemplatesDirectory()
{
	FileSystemBrowser(FileSystemBrowser::ExistingDirectory, this).exec(ui->configurationTemplatesDirectory);
}



void MasterConfigurationPage::populateFeatureComboBox()
{
	ui->computerDoubleClickFeature->addItem(QIcon(), tr("<no feature>"), QUuid());
	ui->computerDoubleClickFeature->insertSeparator(ui->computerDoubleClickFeature->count());

	for (const auto& feature : VeyonCore::featureManager().features())
	{
		if (feature.testFlag(Feature::Flag::Master) &&
			feature.testFlag(Feature::Flag::Meta) == false)
		{
			ui->computerDoubleClickFeature->addItem(QIcon(feature.iconUrl()),
													feature.displayName(),
													feature.uid());
		}
	}
}



void MasterConfigurationPage::updateFeatureLists()
{
	ui->allFeaturesListWidget->setUpdatesEnabled(false);
	ui->disabledFeaturesListWidget->setUpdatesEnabled(false);

	ui->allFeaturesListWidget->clear();
	ui->disabledFeaturesListWidget->clear();

	for (const auto& feature : std::as_const(VeyonCore::featureManager().features()))
	{
		if (feature.testFlag(Feature::Flag::Master) == false ||
			feature.testFlag(Feature::Flag::Meta) ||
			feature == VeyonCore::builtinFeatures().monitoringMode().feature())
		{
			continue;
		}

		auto item = new QListWidgetItem(QIcon(feature.iconUrl()), feature.displayName());
		item->setData(Qt::UserRole, feature.uid().toString());

		if (m_disabledFeatures.contains(feature.uid().toString()))
		{
			ui->disabledFeaturesListWidget->addItem(item);
		}
		else
		{
			ui->allFeaturesListWidget->addItem(item);
		}
	}

	ui->allFeaturesListWidget->setUpdatesEnabled(true);
	ui->disabledFeaturesListWidget->setUpdatesEnabled(true);
}
