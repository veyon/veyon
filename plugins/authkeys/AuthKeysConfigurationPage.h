// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AuthKeysTableModel.h"
#include "ConfigurationPage.h"

namespace Ui {
class AuthKeysConfigurationPage;
}

// clazy:excludeall=ctor-missing-parent-argument
class AuthKeysConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	AuthKeysConfigurationPage();
	~AuthKeysConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private Q_SLOTS:
	void openPublicKeyBaseDir();
	void openPrivateKeyBaseDir();
	void createKeyPair();
	void deleteKey();
	void importKey();
	void exportKey();
	void setAccessGroup();
	void reloadKeyTable();

private:
	QString selectedKey() const;
	void showResultMessage(bool success, const QString& title, const QString& message);

	Ui::AuthKeysConfigurationPage* ui;
	AuthKeysTableModel m_authKeyTableModel;
	const QString m_keyFilesFilter;
};
