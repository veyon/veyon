// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QTcpSocket>

#include "DemoServerProtocol.h"
#include "VariantArrayMessage.h"
#include "VncServerClient.h"


DemoServerProtocol::DemoServerProtocol(const Token& demoAccessToken, QTcpSocket* socket, VncServerClient* client) :
	VncServerProtocol(socket, client),
	m_demoAccessToken(demoAccessToken)
{
}



QVector<RfbVeyonAuth::Type> DemoServerProtocol::supportedAuthTypes() const
{
	return {RfbVeyonAuth::Token};
}



void DemoServerProtocol::processAuthenticationMessage(VariantArrayMessage& message)
{
	if (client()->authType() == RfbVeyonAuth::Token)
	{
		client()->setAuthState(performTokenAuthentication(message));
	}
	else
	{
		client()->setAuthState(VncServerClient::AuthState::Failed);
	}
}



void DemoServerProtocol::performAccessControl()
{
	client()->setAccessControlState(VncServerClient::AccessControlState::Successful);
}



VncServerClient::AuthState DemoServerProtocol::performTokenAuthentication(VariantArrayMessage& message)
{
	switch (client()->authState())
	{
	case VncServerClient::AuthState::Init:
		return VncServerClient::AuthState::Token;

	case VncServerClient::AuthState::Token:
	{
		const auto tokenData = message.read().toByteArray();
		constexpr int MaxTokenSize = 4096;
		if (tokenData.size() > MaxTokenSize)
		{
			vWarning() << "token size exceeds maximum of" << MaxTokenSize << "bytes";
			return VncServerClient::AuthState::Failed;
		}

		if (Token(tokenData) == m_demoAccessToken)
		{
			vDebug() << "SUCCESS";
			return VncServerClient::AuthState::Successful;
		}

		vDebug() << "FAIL";
		return VncServerClient::AuthState::Failed;
	}

	default:
		break;
	}

	return VncServerClient::AuthState::Failed;
}
