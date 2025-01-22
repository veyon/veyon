// Copyright (c) 2019-2025 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QWidget>

namespace Ui {
class AuthLdapConfigurationWidget;
}

class AuthLdapConfiguration;

// clazy:excludeall=ctor-missing-parent-argument
class AuthLdapConfigurationWidget : public QWidget
{
	Q_OBJECT
public:
	AuthLdapConfigurationWidget( AuthLdapConfiguration& configuration );
	~AuthLdapConfigurationWidget() override;

private:
	Ui::AuthLdapConfigurationWidget* ui;
	AuthLdapConfiguration& m_configuration;

};
