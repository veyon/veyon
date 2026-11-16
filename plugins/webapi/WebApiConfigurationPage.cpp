// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/UiMapping.h"
#include "FileSystemBrowser.h"
#include "WebApiConfiguration.h"
#include "WebApiConfigurationPage.h"

#include "ui_WebApiConfigurationPage.h"

WebApiConfigurationPage::WebApiConfigurationPage(WebApiConfiguration& configuration, QWidget* parent) :
	ConfigurationPage(parent),
	ui(new Ui::WebApiConfigurationPage),
	m_configuration(configuration)
{
	ui->setupUi(this);

	connect(ui->browseTlsCertificateFile, &QAbstractButton::clicked, this, [this]() {
		FileSystemBrowser(FileSystemBrowser::ExistingFile, this).exec(ui->tlsCertificateFile);
	});

	connect(ui->browseTlsPrivateKeyFile, &QAbstractButton::clicked, this, [this]() {
		FileSystemBrowser(FileSystemBrowser::ExistingFile, this).exec(ui->tlsPrivateKeyFile);
	});

	Configuration::UiMapping::setFlags(this, Configuration::Property::Flag::Advanced);
}



WebApiConfigurationPage::~WebApiConfigurationPage()
{
	delete ui;
}



void WebApiConfigurationPage::resetWidgets()
{
	FOREACH_HTTP_API_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void WebApiConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_HTTP_API_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}



void WebApiConfigurationPage::applyConfiguration()
{
}
