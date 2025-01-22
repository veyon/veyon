// Copyright (c) 2019-2025 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "ConfigurationPage.h"
#include "LdapCommon.h"

namespace Ui {
class LdapNetworkObjectDirectoryConfigurationPage;
}

class LDAP_COMMON_EXPORT LdapNetworkObjectDirectoryConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	explicit LdapNetworkObjectDirectoryConfigurationPage( QWidget* parent = nullptr );
	~LdapNetworkObjectDirectoryConfigurationPage() override;

	void resetWidgets() override {};
	void connectWidgetsToProperties() override {};
	void applyConfiguration() override {};

private:
	Ui::LdapNetworkObjectDirectoryConfigurationPage *ui;

};
