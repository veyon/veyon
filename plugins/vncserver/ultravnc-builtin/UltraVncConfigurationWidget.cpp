// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/UiMapping.h"
#include "UltraVncConfiguration.h"
#include "UltraVncConfigurationWidget.h"

#include "ui_UltraVncConfigurationWidget.h"

UltraVncConfigurationWidget::UltraVncConfigurationWidget(UltraVncConfiguration& configuration) :
	QWidget(),
	ui(new Ui::UltraVncConfigurationWidget),
	m_configuration(configuration)
{
	ui->setupUi(this);

	Configuration::UiMapping::setFlags(ui->ultraVncMaxCpu, Configuration::Property::Flag::Advanced);
	Configuration::UiMapping::setFlags(ui->ultraVncMaxCpuLabel, Configuration::Property::Flag::Advanced);

	FOREACH_ULTRAVNC_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_ULTRAVNC_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



UltraVncConfigurationWidget::~UltraVncConfigurationWidget()
{
	delete ui;
}
