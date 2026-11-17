// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonObject>
#include <QPointer>

#include "IpcConnection.h"
#include "IpcMessage.h"

#include <functional>
#include <map>

// Synchronous request/response client of a local IPC endpoint (docs/specs/02-ipc.md §6), e.g.
// the session server's connection to veyon-service. There is exactly one connection per child
// process start: once it is lost, every call fails.
class VEYON_CORE_EXPORT IpcClient : public QObject
{
	Q_OBJECT
public:
	using Response = IpcMessage::Response;
	using ResponseHandler = std::function<void(const Response&)>;

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

	// asynchronous call, e.g. for requests the service answers later (directory broker); the
	// handler is called exactly once from the event loop of this object's thread, never before
	// this function has returned; failures are reported like for call()
	void callAsync(const QString& command, const QJsonObject& arguments, const ResponseHandler& handler,
				   int timeout = IpcProtocol::DefaultCallTimeout);

private:
	void handleMessage(const QByteArray& message);
	void finishAsyncCall(int requestId, const Response& response);
	void failAsyncCalls();

	const IpcProtocol::Role m_role;
	const quint32 m_sessionId;
	QPointer<IpcConnection> m_connection;
	int m_nextRequestId = 1;
	QList<QByteArray> m_receivedMessages;
	// one slot stays free for synchronous calls
	static constexpr int MaximumAsyncCalls = IpcProtocol::MaximumInFlightRequests - 1;
	std::map<int, ResponseHandler> m_asyncCalls;
};
