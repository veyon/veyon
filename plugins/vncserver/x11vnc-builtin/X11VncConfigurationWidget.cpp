// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/UiMapping.h"
#include "X11VncConfiguration.h"
#include "X11VncConfigurationWidget.h"

#include "ui_X11VncConfigurationWidget.h"

X11VncConfigurationWidget::X11VncConfigurationWidget(X11VncConfiguration& configuration, QWidget* parent) :
	QWidget(parent),
	ui(new Ui::X11VncConfigurationWidget),
	m_configuration(configuration)
{
	ui->setupUi(this);

	// all properties of this widget are advanced - mark the whole widget as
	// advanced so its static labels are hidden together with the input fields
	// in standard view mode
	Configuration::UiMapping::setFlags(this, Configuration::Property::Flag::Advanced);

	FOREACH_X11VNC_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_X11VNC_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



X11VncConfigurationWidget::~X11VncConfigurationWidget()
{
	delete ui;
}
