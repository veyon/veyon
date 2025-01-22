// Copyright (c) 2019-2025 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "LdapNetworkObjectDirectoryConfigurationPage.h"

#include "ui_LdapNetworkObjectDirectoryConfigurationPage.h"

LdapNetworkObjectDirectoryConfigurationPage::LdapNetworkObjectDirectoryConfigurationPage( QWidget* parent ) :
	ConfigurationPage( parent ),
	ui(new Ui::LdapNetworkObjectDirectoryConfigurationPage)
{
	ui->setupUi(this);
}



LdapNetworkObjectDirectoryConfigurationPage::~LdapNetworkObjectDirectoryConfigurationPage()
{
	delete ui;
}
