// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMutex>
#include <QSemaphore>
#include <QSet>
#include <QThread>

#include "IpcServer.h"
#include "ServiceIpcCommandHandler.h"

#include <map>

// Service side of the local IPC with the session servers (docs/specs/02-ipc.md §1–§6): one
// endpoint per started server, created before the server starts, served in a dedicated thread.
// All public functions are thread-safe.
class ServiceDataManager : public QThread
{
	Q_OBJECT
public:
	struct ServerEndpoint
	{
		QString name;
		// "VEYONIPC1 <name> <hex64>\n", to be written to the server's stdin only
		VeyonCrypto::SecureBuffer credentialsLine;
	};

	explicit ServiceDataManager(QObject* parent = nullptr);
	~ServiceDataManager() override;

	// see ServiceIpcCommandHandler::Client; hasLoggedOnUser is called in the IPC thread and must
	// return true if unsure
	[[nodiscard]] std::optional<ServerEndpoint> createServerEndpoint(int sessionId, const QString& platformSessionId,
																	 const IpcServer::PeerPolicy& peerPolicy,
																	 const std::function<bool()>& hasLoggedOnUser);
	void setServerProcessId(const QString& endpointName, IpcServer::ProcessId processId);
	void removeServerEndpoint(const QString& endpointName);

	bool hasServerEndpointFailed(const QString& endpointName) const;

Q_SIGNALS:
	// the endpoint was closed after too many failed connection attempts: the server is
	// considered failed and has to be restarted
	void serverEndpointFailed(const QString& endpointName);

protected:
	void run() override;

private:
	struct Endpoint
	{
		IpcServer* server = nullptr;
		ServiceIpcCommandHandler::Client client{};
	};

	template<typename Function>
	void runInIpcThread(Function&& function);

	void handleRequest(const QString& endpointName, IpcConnection* connection, const QByteArray& message);

	static std::optional<VeyonCrypto::SecureBuffer> externalVncServerPassword();

	QObject* m_context = nullptr;
	QSemaphore m_contextReady;

	// accessed in the IPC thread only
	ServiceIpcCommandHandler m_commandHandler;
	std::map<QString, Endpoint> m_endpoints;

	mutable QMutex m_failedEndpointsMutex;
	QSet<QString> m_failedEndpoints;
};
