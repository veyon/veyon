// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWidget>

namespace Ui {
class UltraVncConfigurationWidget;
}

class UltraVncConfiguration;

class UltraVncConfigurationWidget : public QWidget
{
	Q_OBJECT

public:
	UltraVncConfigurationWidget(UltraVncConfiguration& configuration);
	~UltraVncConfigurationWidget() override;

private:
	Ui::UltraVncConfigurationWidget* ui;
	UltraVncConfiguration& m_configuration;
};
