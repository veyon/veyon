// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWidget>

namespace Ui {
class X11VncConfigurationWidget;
}

class X11VncConfiguration;

class X11VncConfigurationWidget : public QWidget
{
	Q_OBJECT

public:
	explicit X11VncConfigurationWidget(X11VncConfiguration& configuration, QWidget* parent = nullptr);
	~X11VncConfigurationWidget() override;

private:
	Ui::X11VncConfigurationWidget* ui;
	X11VncConfiguration& m_configuration;
};
