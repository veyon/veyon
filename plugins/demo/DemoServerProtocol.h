// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VncServerClient.h"
#include "VncServerProtocol.h"

// clazy:excludeall=copyable-polymorphic

class DemoServerProtocol : public VncServerProtocol
{
public:
	using Token = CryptoCore::PlaintextPassword;

	DemoServerProtocol(const Token& demoAccessToken, QTcpSocket* socket, VncServerClient* client);

protected:
	QVector<RfbVeyonAuth::Type> supportedAuthTypes() const override;
	void processAuthenticationMessage(VariantArrayMessage& message) override;
	void performAccessControl() override;

private:
	VncServerClient::AuthState performTokenAuthentication(VariantArrayMessage& message);

	const Token m_demoAccessToken;
};
