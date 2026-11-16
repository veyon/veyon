// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMutex>
#include <QStringList>

#include "RfbVeyonAuth.h"
#include "VncServerClient.h"

class VariantArrayMessage;

class ServerAuthenticationManager : public QObject
{
	Q_OBJECT
public:
	enum class AuthResult {
		Successful,
		Failed
	};
	Q_ENUM(AuthResult)

	explicit ServerAuthenticationManager(QObject* parent);

	QVector<RfbVeyonAuth::Type> supportedAuthTypes() const;

	void processAuthenticationMessage(VncServerClient* client,
									  VariantArrayMessage& message);


Q_SIGNALS:
	void finished(VncServerClient* client);

private:
	VncServerClient::AuthState performKeyAuthentication(VncServerClient* client, VariantArrayMessage& message);
	VncServerClient::AuthState performLogonAuthentication(VncServerClient* client, VariantArrayMessage& message);
	VncServerClient::AuthState performTokenAuthentication(VncServerClient* client, VariantArrayMessage& message);
};
