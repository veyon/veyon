// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QTimer>

#include "IpcChannel.h"
#include "IpcEndpointCredentials.h"

class QLocalSocket;

// IpcChannel on top of a QLocalSocket (Unix domain socket / Windows named pipe). Emits closed()
// exactly once when the channel fails, the peer disconnects or close() is called.
class VEYON_CORE_EXPORT IpcConnection : public QObject
{
	Q_OBJECT
public:
	// takes ownership of the socket
	IpcConnection(QLocalSocket* socket, IpcChannel::Side side, IpcProtocol::Role role, quint32 sessionId,
				  IpcEndpointCredentials&& credentials, IpcChannel::NonceHistory* nonceHistory = nullptr,
				  QObject* parent = nullptr);
	~IpcConnection() override;

	// client side: connects to an endpoint created by IpcServer; on Windows the pipe is opened
	// with identification-only impersonation (SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION) and
	// without requesting FILE_CREATE_PIPE_INSTANCE; returns nullptr on failure
	[[nodiscard]] static QLocalSocket* connectToEndpoint(const QString& directory, const QString& endpointName,
														 int timeout);

	// starts the handshake (client: sends message 1)
	void start();

	bool isEstablished() const
	{
		return m_channel.isEstablished();
	}

	bool isClosed() const
	{
		return m_closed;
	}

	[[nodiscard]] bool sendMessage(const QByteArray& message);

	// blocking helpers for synchronous callers (process events of this connection only)
	[[nodiscard]] bool waitForEstablished(int timeout);
	[[nodiscard]] bool waitForReadyRead(int timeout);

	void close();

Q_SIGNALS:
	void established();
	void messageReceived(const QByteArray& message);
	void closed();

private:
	static constexpr auto HandshakeTimerInterval = 100;
	static constexpr auto FrameTimerInterval = 1000;
	static constexpr qint64 ReadChunkSize = 4096;

	void readData();
	void processOutputs(const IpcChannel::Outputs& outputs);
	void checkTimeout();

	QLocalSocket* m_socket;
	IpcChannel m_channel;
	QTimer m_timeoutTimer{this};
	bool m_closed = false;
};
