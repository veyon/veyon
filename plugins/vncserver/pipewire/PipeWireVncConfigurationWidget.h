// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWidget>

namespace Ui {
class PipeWireVncConfigurationWidget;
}

class PipeWireVncConfiguration;

class PipeWireVncConfigurationWidget : public QWidget
{
	Q_OBJECT

public:
	explicit PipeWireVncConfigurationWidget(PipeWireVncConfiguration& configuration, QWidget* parent = nullptr);
	~PipeWireVncConfigurationWidget() override;

private:
	Ui::PipeWireVncConfigurationWidget* ui;
	PipeWireVncConfiguration& m_configuration;
};
