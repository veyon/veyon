// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/UiMapping.h"
#include "LinuxPlatformConfiguration.h"
#include "LinuxPlatformConfigurationPage.h"
#include "VeyonConfiguration.h"

#include "ui_LinuxPlatformConfigurationPage.h"


LinuxPlatformConfigurationPage::LinuxPlatformConfigurationPage() :
	ui(new Ui::LinuxPlatformConfigurationPage),
	m_configuration(&VeyonCore::config())
{
	ui->setupUi(this);

	Configuration::UiMapping::setFlags(this, Configuration::Property::Flag::Advanced);
}



LinuxPlatformConfigurationPage::~LinuxPlatformConfigurationPage()
{
	delete ui;
}



void LinuxPlatformConfigurationPage::resetWidgets()
{
	FOREACH_LINUX_PLATFORM_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void LinuxPlatformConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_LINUX_PLATFORM_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void LinuxPlatformConfigurationPage::applyConfiguration()
{
}
