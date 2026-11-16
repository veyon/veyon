// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "VeyonConfiguration.h"
#include "WindowsPlatformConfiguration.h"
#include "WindowsPlatformConfigurationPage.h"
#include "Configuration/UiMapping.h"

#include "ui_WindowsPlatformConfigurationPage.h"


WindowsPlatformConfigurationPage::WindowsPlatformConfigurationPage() :
	ui(new Ui::WindowsPlatformConfigurationPage),
	m_configuration(&VeyonCore::config())
{
	ui->setupUi(this);

	Configuration::UiMapping::setFlags(this, Configuration::Property::Flag::Advanced);
}



WindowsPlatformConfigurationPage::~WindowsPlatformConfigurationPage()
{
	delete ui;
}



void WindowsPlatformConfigurationPage::resetWidgets()
{
	FOREACH_WINDOWS_PLATFORM_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY)
}



void WindowsPlatformConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_WINDOWS_PLATFORM_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}



void WindowsPlatformConfigurationPage::applyConfiguration()
{
}
