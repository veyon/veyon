// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWidget>

namespace Ui {
class ExternalVncServerConfigurationWidget;
}

class ExternalVncServerConfiguration;

class ExternalVncServerConfigurationWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ExternalVncServerConfigurationWidget(ExternalVncServerConfiguration& configuration, QWidget* parent = nullptr);
	~ExternalVncServerConfigurationWidget() override;

private:
	Ui::ExternalVncServerConfigurationWidget* ui;
	ExternalVncServerConfiguration& m_configuration;
};
