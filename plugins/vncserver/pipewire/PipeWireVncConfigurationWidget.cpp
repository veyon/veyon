// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/UiMapping.h"
#include "PipeWireVncConfiguration.h"
#include "PipeWireVncConfigurationWidget.h"

#include "ui_PipeWireVncConfigurationWidget.h"

PipeWireVncConfigurationWidget::PipeWireVncConfigurationWidget(PipeWireVncConfiguration& configuration, QWidget* parent) :
	QWidget(parent),
	ui(new Ui::PipeWireVncConfigurationWidget),
	m_configuration(configuration)
{
	ui->setupUi(this);

	// all properties of this widget are advanced - mark the whole widget as
	// advanced so its static labels are hidden together with the input fields
	// in standard view mode
	Configuration::UiMapping::setFlags(this, Configuration::Property::Flag::Advanced);

	FOREACH_PIPEWIREVNC_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_PIPEWIREVNC_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



PipeWireVncConfigurationWidget::~PipeWireVncConfigurationWidget()
{
	delete ui;
}
