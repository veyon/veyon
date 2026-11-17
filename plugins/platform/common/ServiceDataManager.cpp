// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QPointer>

#include "DirectoryBrokerClient.h"
#include "DirectoryBrokerService.h"
#include "IpcConnection.h"
#include "NetworkObjectDirectoryManager.h"
#include "SecretStore.h"
#include "ServiceDataManager.h"
#include "TlsHostCredentials.h"
#include "UserGroupsBackendManager.h"
#include "VeyonConfiguration.h"
#include "pki/TlsConfiguration.h"


// called in the directory worker thread; the managers' plugin lists are not modified after start
static bool isCentralDirectoryBrokerConfigured()
{
	if (DirectoryBrokerClient::isLocalHost(VeyonCore::config().directoryBrokerHost()))
	{
		return false;
	}

	// 04-directory-broker.md §3a: requires a verified TLS connection and KeyFileBound
	// authentication to the central host, which are not available yet - fail closed
	vCritical() << "central directory broker mode (DirectoryBroker/Host) is not supported by this version - "
				   "directory and user groups are unavailable";
	return true;
}



static NetworkObjectDirectory* createBrokerDirectory()
{
	if (isCentralDirectoryBrokerConfigured())
	{
		return nullptr;
	}

	return VeyonCore::networkObjectDirectoryManager().createDirectory(VeyonCore::config().networkObjectDirectoryPlugin(), nullptr);
}



static UserGroupsBackendInterface* brokerUserGroupsBackend()
{
	if (isCentralDirectoryBrokerConfigured())
	{
		return nullptr;
	}

	return VeyonCore::userGroupsBackendManager().configuredLocalBackend();
}


ServiceDataManager::ServiceDataManager(QObject* parent) :
	QThread(parent)
{
	vDebug();

	m_commandHandler.setExternalVncServerPasswordProvider(&ServiceDataManager::externalVncServerPassword);
	m_commandHandler.setTlsHostKeyProvider(&ServiceDataManager::tlsHostKey);

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
	const std::function<bool()>& hasLoggedOnUser, bool tlsHostKeyAllowed)
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
		endpoint.client.tlsHostKeyAllowed = tlsHostKeyAllowed;

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

		if (m_directoryBrokerService)
		{
			m_directoryBrokerService->removeClient(endpointName);
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



void ServiceDataManager::setAuthenticator(const ServiceIpcCommandHandler::Authenticator& authenticator,
										  const std::shared_ptr<AuthenticationLimiter>& limiter)
{
	runInIpcThread([&]() { m_commandHandler.setAuthenticator(authenticator, limiter); });
}



void ServiceDataManager::run()
{
	QObject context;
	m_context = &context;

	DirectoryBrokerService directoryBrokerService(&createBrokerDirectory, &brokerUserGroupsBackend);
	m_directoryBrokerService = &directoryBrokerService;

	m_contextReady.release();

	exec();

	for (auto& endpoint : m_endpoints)
	{
		delete endpoint.second.server;
	}
	m_endpoints.clear();

	m_directoryBrokerService = nullptr;
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

	if (DirectoryBrokerMessage::isBrokerCommand(request->command))
	{
		handleDirectoryBrokerRequest(endpointName, connection, *request);
		return;
	}

	// Authenticate is answered later: the connection may be gone by then
	m_commandHandler.handleRequest(it->second.client, *request, m_context,
								   [connection = QPointer<IpcConnection>(connection)](const IpcMessage::Response& response) {
		if (connection && connection->sendMessage(IpcMessage::encodeResponse(response)) == false)
		{
			connection->close();
		}
	});
}



void ServiceDataManager::handleDirectoryBrokerRequest(const QString& endpointName, IpcConnection* connection,
													  const IpcMessage::Request& request)
{
	const QPointer<IpcConnection> replyConnection(connection);
	const auto ipcRequestId = request.id;

	// one broker client per session server endpoint (limits, cursors, round-robin queue)
	m_directoryBrokerService->handleRequest(endpointName, DirectoryBrokerMessage::fromIpcRequest(request),
											[replyConnection, ipcRequestId](const DirectoryBrokerMessage::Reply& reply) {
		if (replyConnection.isNull() || replyConnection->isClosed())
		{
			return;
		}

		auto message = IpcMessage::encodeResponse(DirectoryBrokerMessage::toIpcResponse(ipcRequestId, reply));
		if (message.size() > IpcProtocol::MaximumRecordPlaintextSize)
		{
			vCritical() << "directory broker reply too large for IPC";
			message = IpcMessage::encodeResponse(
				DirectoryBrokerMessage::toIpcResponse(ipcRequestId, DirectoryBrokerMessage::error(0, DirectoryBrokerProtocol::Error::TooLarge)));
		}

		if (replyConnection->sendMessage(message) == false)
		{
			replyConnection->close();
		}
	});
}



std::optional<Veyon::Crypto::SecureBuffer> ServiceDataManager::externalVncServerPassword()
{
	// plugin UID of ExternalVncServer (plugins/vncserver/external)
	static const QUuid externalVncServerPluginUid(QStringLiteral("67dfc1c1-8f37-4539-a298-16e74e34fd8b"));

	if (VeyonCore::config().vncServerPlugin() != externalVncServerPluginUid)
	{
		return {};
	}

	// fixed key of the ExternalVncServer/Password property (SecretReference, 03-secretstore-config.md
	// §4.2); the configuration value never selects the key
	auto password = SecretStore::read(QStringLiteral("vncserver-external/password"));
	if (password.has_value() == false || password->isEmpty())
	{
		return {};
	}

	return password;
}



std::optional<ServiceIpcCommandHandler::TlsHostKey> ServiceDataManager::tlsHostKey()
{
	// 02-ipc.md §6: refused with TLS disabled or without a valid host certificate
	if (TlsConfiguration::serverMode(VeyonCore::config()) == VeyonCore::TlsServerMode::Disabled)
	{
		return {};
	}

	auto credentials = TlsHostCredentials::load(VeyonCore::config());
	if (credentials.has_value() == false)
	{
		return {};
	}

	return ServiceIpcCommandHandler::TlsHostKey{std::move(credentials->privateKeyDer),
												credentials->certificateChainPem};
}
