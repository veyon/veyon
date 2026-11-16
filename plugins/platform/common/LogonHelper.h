// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CryptoCore.h"

// clazy:exclude=copyable-polymorphic

class LogonHelper : public QObject
{
	Q_OBJECT
public:
	using Password = CryptoCore::SecureArray;

	LogonHelper(QObject* parent = nullptr);

	bool prepare(const QString& username, const Password& password);

private:
	void checkPendingLogonTasks();
};
