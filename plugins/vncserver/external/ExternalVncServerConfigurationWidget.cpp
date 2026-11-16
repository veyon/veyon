// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/UiMapping.h"
#include "ExternalVncServerConfiguration.h"
#include "ExternalVncServerConfigurationWidget.h"

#include "ui_ExternalVncServerConfigurationWidget.h"

ExternalVncServerConfigurationWidget::ExternalVncServerConfigurationWidget(ExternalVncServerConfiguration& configuration,
																		   QWidget* parent) :
	QWidget(parent),
	ui(new Ui::ExternalVncServerConfigurationWidget),
	m_configuration(configuration)
{
	ui->setupUi(this);

	FOREACH_EXTERNAL_VNC_SERVER_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_EXTERNAL_VNC_SERVER_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



ExternalVncServerConfigurationWidget::~ExternalVncServerConfigurationWidget()
{
	delete ui;
}
