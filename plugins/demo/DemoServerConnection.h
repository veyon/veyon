// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DemoServerProtocol.h"

class DemoServer;

// clazy:excludeall=ctor-missing-parent-argument

// the demo server creates an instance of this class for each client connection,
// i.e. each client is connected to a different server thread for best
// performance
class DemoServerConnection : public QThread
{
	Q_OBJECT
public:
	using Password = CryptoCore::PlaintextPassword;

	enum {
		ProtocolRetryTime = 250,
	};

	DemoServerConnection(DemoServer* demoServer, const Password& demoAccessToken, quintptr socketDescriptor);
	~DemoServerConnection() = default;

	qsizetype fbuSequenceNumber() const
	{
		return m_fbuSequenceNumber;
	}

Q_SIGNALS:
	void synchronizationLost(int epochId);

private:
	void run() override;

	void processClient(); // clazy:exclude=thread-with-slots
	void sendFramebufferUpdate();

	bool receiveClientMessage();

	const Password m_demoAccessToken;
	DemoServer* m_demoServer;

	quintptr m_socketDescriptor;
	QTcpSocket* m_socket{nullptr};

	VncServerClient m_vncServerClient;
	DemoServerProtocol* m_serverProtocol{nullptr};

	const QMap<int, int> m_rfbClientToServerMessageSizes;

	QAtomicInteger<qsizetype> m_fbuSequenceNumber = 0;
	int m_epochId;

	const int m_framebufferUpdateInterval;
};
