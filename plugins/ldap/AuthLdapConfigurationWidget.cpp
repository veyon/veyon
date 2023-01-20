// Copyright (c) 2019-2023 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "AuthLdapConfiguration.h"
#include "AuthLdapConfigurationWidget.h"
#include "VeyonConfiguration.h"
#include "Configuration/UiMapping.h"

#include "ui_AuthLdapConfigurationWidget.h"


AuthLdapConfigurationWidget::AuthLdapConfigurationWidget( AuthLdapConfiguration& configuration ) :
	QWidget( QApplication::activeWindow() ),
	ui( new Ui::AuthLdapConfigurationWidget ),
	m_configuration( configuration )
{
	ui->setupUi(this);

	FOREACH_AUTH_LDAP_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
	FOREACH_AUTH_LDAP_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY)
}



AuthLdapConfigurationWidget::~AuthLdapConfigurationWidget()
{
	delete ui;
}
