// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonObject>
#include <QPointer>

#include "IpcConnection.h"
#include "IpcMessage.h"

// Synchronous request/response client of a local IPC endpoint (docs/specs/02-ipc.md §6), e.g.
// the session server's connection to veyon-service. There is exactly one connection per child
// process start: once it is lost, every call fails.
class VEYON_CORE_EXPORT IpcClient : public QObject
{
	Q_OBJECT
public:
	using Response = IpcMessage::Response;

	IpcClient(IpcProtocol::Role role, quint32 sessionId, QObject* parent = nullptr);
	~IpcClient() override;

	// directory: Linux socket directory of the endpoint (ignored on Windows)
	[[nodiscard]] bool connectAndHandshake(const QString& directory, IpcEndpointCredentials&& credentials,
										   int timeout = IpcProtocol::HandshakeTimeout);

	bool isConnected() const;

	// failed calls (no connection, timeout, malformed response) return ok = false with an empty
	// error code
	[[nodiscard]] Response call(QLatin1String command, const QJsonObject& arguments = {},
								int timeout = IpcProtocol::DefaultCallTimeout);

private:
	const IpcProtocol::Role m_role;
	const quint32 m_sessionId;
	QPointer<IpcConnection> m_connection;
	int m_nextRequestId = 1;
	QList<QByteArray> m_receivedMessages;
};
