// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CryptoCore.h"
#include "VncServerSocket.h"

class QTcpSocket;
class VncProxyConnection;

// clazy:excludeall=copyable-polymorphic

class VncProxyConnectionFactory
{
public:
	using Password = CryptoCore::SecureArray;

	virtual ~VncProxyConnectionFactory() = default;

	virtual VncProxyConnection* createVncProxyConnection(QTcpSocket* clientSocket,
														 const VncServerEndpoint& vncServerEndpoint,
														 const Password& vncServerPassword,
														 QObject* parent) = 0;
};
