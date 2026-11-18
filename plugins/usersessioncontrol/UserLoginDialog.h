// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QObject>
#include "CryptoCore.h"
class QWidget;
class UserLoginDialog : public QObject
{
	Q_OBJECT
public:
	explicit UserLoginDialog(QWidget* parent);
	int exec();
	QString username() const { return m_username; }
	CryptoCore::PlaintextPassword password() const { return m_password; }
private:
	QString m_username;
	CryptoCore::PlaintextPassword m_password;
};
