// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QHostAddress>

#include "Configuration/UiMapping.h"
#include "InternetBlockAddressRanges.h"
#include "InternetBlockConfiguration.h"
#include "InternetBlockConfigurationPage.h"

#include "ui_InternetBlockConfigurationPage.h"


InternetBlockConfigurationPage::InternetBlockConfigurationPage(InternetBlockConfiguration& configuration, QWidget* parent) :
	ConfigurationPage(parent),
	ui(new Ui::InternetBlockConfigurationPage),
	m_configuration(configuration)
{
	ui->setupUi(this);

	Configuration::UiMapping::setFlags(this, Configuration::Property::Flag::Advanced);

	connect(ui->allowedNetworks, &QLineEdit::textChanged, this, &InternetBlockConfigurationPage::validateAllowedNetworks);
}



InternetBlockConfigurationPage::~InternetBlockConfigurationPage()
{
	delete ui;
}



void InternetBlockConfigurationPage::resetWidgets()
{
	FOREACH_INTERNET_BLOCK_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	validateAllowedNetworks();
}



void InternetBlockConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_INTERNET_BLOCK_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}



void InternetBlockConfigurationPage::applyConfiguration()
{
}



void InternetBlockConfigurationPage::validateAllowedNetworks()
{
	QStringList invalid;
	QStringList ipv6;
	InternetBlockAddressRanges::parseIPv4Networks(
		InternetBlockAddressRanges::splitNetworkList(ui->allowedNetworks->text()), &invalid, &ipv6);

	if (invalid.isEmpty())
	{
		ui->allowedNetworksStatus->setText({});
		ui->allowedNetworksStatus->hide();
	}
	else
	{
		ui->allowedNetworksStatus->setText(tr("Invalid entries (will be ignored): %1").arg(invalid.join(QStringLiteral(", "))));
		ui->allowedNetworksStatus->show();
	}
}
