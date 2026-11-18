// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QObject>
#include "AuthenticationCredentials.h"
class QWidget;
class VEYON_CORE_EXPORT PasswordDialog : public QObject
{
	Q_OBJECT
public:
	explicit PasswordDialog(QWidget* parent = nullptr);
	int exec();
	QString username() const { return m_username; }
	CryptoCore::PlaintextPassword password() const { return m_password; }
	AuthenticationCredentials credentials() const;
private:
	QString m_username;
	CryptoCore::PlaintextPassword m_password;
};
