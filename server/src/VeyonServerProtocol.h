// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VncServerProtocol.h"

class QTcpSocket;

class ServerAuthenticationManager;
class ServerAccessControlManager;

// clazy:excludeall=copyable-polymorphic

class VeyonServerProtocol : public VncServerProtocol
{
public:
	VeyonServerProtocol(QTcpSocket* socket,
						VncServerClient* client,
						ServerAuthenticationManager& serverAuthenticationManager,
						ServerAccessControlManager& serverAccessControlManager);

protected:
	QVector<RfbVeyonAuth::Type> supportedAuthTypes() const override;
	void processAuthenticationMessage(VariantArrayMessage& message) override;
	void performAccessControl() override;

private:
	ServerAuthenticationManager& m_serverAuthenticationManager;
	ServerAccessControlManager& m_serverAccessControlManager;
};
