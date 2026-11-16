// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/UiMapping.h"
#include "DemoConfiguration.h"
#include "DemoConfigurationPage.h"

#include "ui_DemoConfigurationPage.h"

DemoConfigurationPage::DemoConfigurationPage(DemoConfiguration& configuration, QWidget* parent) :
	ConfigurationPage(parent),
	ui(new Ui::DemoConfigurationPage),
	m_configuration(configuration)
{
	ui->setupUi(this);

	Configuration::UiMapping::setFlags(this, Configuration::Property::Flag::Advanced);
}



DemoConfigurationPage::~DemoConfigurationPage()
{
	delete ui;
}



void DemoConfigurationPage::resetWidgets()
{
	FOREACH_DEMO_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void DemoConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_DEMO_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}



void DemoConfigurationPage::applyConfiguration()
{
}
