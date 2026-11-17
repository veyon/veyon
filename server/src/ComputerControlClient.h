// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QElapsedTimer>

#include "VeyonServerProtocol.h"
#include "VncClientProtocol.h"
#include "VncProxyConnection.h"
#include "VncServerClient.h"

class ComputerControlServer;

class ComputerControlClient : public VncProxyConnection
{
	Q_OBJECT
public:
	using Password = CryptoCore::SecureArray;

	ComputerControlClient(ComputerControlServer* server,
						  QTcpSocket* clientSocket,
						  const VncServerEndpoint& vncServerEndpoint,
						  const Password& vncServerPassword,
						  QObject* parent);
	~ComputerControlClient() override;

	bool receiveClientMessage() override;

	VncServerClient* serverClient()
	{
		return &m_serverClient;
	}

	void setMinimumFramebufferUpdateInterval(int interval);

protected:
	VncClientProtocol& clientProtocol() override
	{
		return m_clientProtocol;
	}

	VncServerProtocol& serverProtocol() override
	{
		return m_serverProtocol;
	}

private:
	ComputerControlServer* m_server;

	VncServerClient m_serverClient;

	VeyonServerProtocol m_serverProtocol;
	VncClientProtocol m_clientProtocol;

	int m_minimumFramebufferUpdateInterval{-1};
	QElapsedTimer m_framebufferUpdateTimer;
};
