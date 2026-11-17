// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/Password.h"
#include "IpcConnection.h"
#include "ServiceDataManager.h"
#include "VeyonConfiguration.h"


ServiceDataManager::ServiceDataManager(QObject* parent) :
	QThread(parent)
{
	vDebug();

	m_commandHandler.setExternalVncServerPasswordProvider(&ServiceDataManager::externalVncServerPassword);

	start();

	// endpoints can be created as soon as the constructor returns
	m_contextReady.acquire();
}



ServiceDataManager::~ServiceDataManager()
{
	vDebug();

	quit();
	wait();
}



std::optional<ServiceDataManager::ServerEndpoint> ServiceDataManager::createServerEndpoint(
	int sessionId, const QString& platformSessionId, const IpcServer::PeerPolicy& peerPolicy,
	const std::function<bool()>& hasLoggedOnUser)
{
	std::optional<ServerEndpoint> serverEndpoint;

	runInIpcThread([&]() {
		const auto directory = IpcServer::serviceEndpointDirectory();
		if (directory.isEmpty() == false &&
			IpcServer::prepareDirectory(directory, IpcServer::DirectoryType::Shared) == false)
		{
			vCritical() << "refusing to start servers without a trusted endpoint directory";
			return;
		}

		auto server = new IpcServer(IpcProtocol::Role::SessionServer, quint32(sessionId), m_context);
		if (sessionId < 0 || server->listen(directory, peerPolicy) == false)
		{
			vCritical() << "failed to create IPC endpoint for session" << sessionId;
			delete server;
			return;
		}

		const auto endpointName = server->endpointName();

		connect(server, &IpcServer::failed, m_context, [this, endpointName]() {
			m_failedEndpointsMutex.lock();
			m_failedEndpoints.insert(endpointName);
			m_failedEndpointsMutex.unlock();

			Q_EMIT serverEndpointFailed(endpointName);
		});

		connect(server, &IpcServer::connectionEstablished, m_context, [this, endpointName](IpcConnection* connection) {
			connect(connection, &IpcConnection::messageReceived, m_context,
					[this, endpointName, connection](const QByteArray& message) {
				handleRequest(endpointName, connection, message);
			});
			connect(connection, &IpcConnection::closed, m_context, [endpointName]() {
				vDebug() << "server disconnected from" << endpointName;
			});
		});

		auto& endpoint = m_endpoints[endpointName];
		endpoint.server = server;
		endpoint.client.sessionId = sessionId;
		endpoint.client.platformSessionId = platformSessionId;
		endpoint.client.hasLoggedOnUser = hasLoggedOnUser;

		serverEndpoint = ServerEndpoint{endpointName, server->credentialsLine()};
	});

	return serverEndpoint;
}



void ServiceDataManager::setServerProcessId(const QString& endpointName, IpcServer::ProcessId processId)
{
	runInIpcThread([&]() {
		const auto it = m_endpoints.find(endpointName);
		if (it != m_endpoints.end())
		{
			it->second.server->setExpectedProcessId(processId);
		}
	});
}



void ServiceDataManager::removeServerEndpoint(const QString& endpointName)
{
	if (endpointName.isEmpty())
	{
		return;
	}

	runInIpcThread([&]() {
		const auto it = m_endpoints.find(endpointName);
		if (it != m_endpoints.end())
		{
			delete it->second.server;
			m_endpoints.erase(it);
		}
	});

	QMutexLocker locker(&m_failedEndpointsMutex);
	m_failedEndpoints.remove(endpointName);
}



bool ServiceDataManager::hasServerEndpointFailed(const QString& endpointName) const
{
	QMutexLocker locker(&m_failedEndpointsMutex);
	return m_failedEndpoints.contains(endpointName);
}



void ServiceDataManager::run()
{
	QObject context;
	m_context = &context;
	m_contextReady.release();

	exec();

	for (auto& endpoint : m_endpoints)
	{
		delete endpoint.second.server;
	}
	m_endpoints.clear();

	m_context = nullptr;
}



template<typename Function>
void ServiceDataManager::runInIpcThread(Function&& function)
{
	if (QThread::currentThread() == this)
	{
		function();
	}
	else if (m_context)
	{
		QMetaObject::invokeMethod(m_context, std::forward<Function>(function), Qt::BlockingQueuedConnection);
	}
}



void ServiceDataManager::handleRequest(const QString& endpointName, IpcConnection* connection, const QByteArray& message)
{
	const auto it = m_endpoints.find(endpointName);
	if (it == m_endpoints.end())
	{
		connection->close();
		return;
	}

	const auto request = IpcMessage::decodeRequest(message);
	if (request.has_value() == false)
	{
		vWarning() << "closing connection at" << endpointName << "after malformed request";
		connection->close();
		return;
	}

	const auto response = m_commandHandler.handleRequest(it->second.client, *request);
	if (connection->sendMessage(IpcMessage::encodeResponse(response)) == false)
	{
		connection->close();
	}
}



std::optional<VeyonCrypto::SecureBuffer> ServiceDataManager::externalVncServerPassword()
{
	// plugin UID of ExternalVncServer (plugins/vncserver/external)
	static const QUuid externalVncServerPluginUid(QStringLiteral("67dfc1c1-8f37-4539-a298-16e74e34fd8b"));

	if (VeyonCore::config().vncServerPlugin() != externalVncServerPluginUid)
	{
		return {};
	}

	const auto password = Configuration::Password::fromEncrypted(
							  VeyonCore::config().value(QStringLiteral("Password"), QStringLiteral("ExternalVncServer"), {}).toString())
							  .plainText();
	if (password.isEmpty())
	{
		return {};
	}

	return VeyonCrypto::SecureBuffer(password.constData(), password.size());
}
