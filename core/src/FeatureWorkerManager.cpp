// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QDir>
#include <QThread>
#include <QTimer>

#include "FeatureManager.h"
#include "FeatureWorkerManager.h"
#include "Filesystem.h"
#include "IpcConnection.h"
#include "PlatformCoreFunctions.h"
#include "PlatformUserFunctions.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"

#include <memory>

// clazy:excludeall=detaching-member

FeatureWorkerManager::FeatureWorkerManager(VeyonServerInterface& server, QObject* parent) :
	QObject(parent),
	m_server(server)
{
	auto pendingMessagesTimer = new QTimer(this);
	connect(pendingMessagesTimer, &QTimer::timeout, this, &FeatureWorkerManager::sendPendingMessages);

	pendingMessagesTimer->start(100);

	m_sessionWorkerRetryTimer.setInterval(UnmanagedSessionProcessRetryInterval);
	connect(&m_sessionWorkerRetryTimer, &QTimer::timeout,
			this, &FeatureWorkerManager::retryPendingSessionWorkerMessages);
}



FeatureWorkerManager::~FeatureWorkerManager()
{
	// properly shutdown all worker processes
	while (m_workers.isEmpty() == false)
	{
		stopWorker(m_workers.firstKey());
	}
}



bool FeatureWorkerManager::startManagedSystemWorker(Feature::Uid featureUid)
{
	if (thread() != QThread::currentThread())
	{
		vCritical() << "thread mismatch for feature" << featureUid;
		return false;
	}

	stopWorker(featureUid);

	auto endpoint = createWorkerEndpoint(featureUid, WorkerType::ManagedSystemWorker);
	if (endpoint == nullptr)
	{
		return false;
	}

	Worker worker;
	worker.endpoint = endpoint;
	worker.endpointName = endpoint->endpointName();

	worker.process = new QProcess;
	worker.process->setProcessChannelMode(QProcess::ForwardedChannels);

	// a worker that terminates before connecting should not remain registered;
	// otherwise, `isWorkerRunning()` remains true and the feature appears active
	// until the server is restarted
	const auto endpointName = worker.endpointName;

	connect(worker.process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
			this, [=, this]() { removeStaleWorker(featureUid, endpointName); });

	connect(worker.process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
			worker.process, &QProcess::deleteLater);

	// register the PID before the worker can read the credentials (the endpoint only accepts it)
	auto credentialsLine = std::make_shared<Veyon::Crypto::SecureBuffer>(endpoint->credentialsLine());
	const QPointer<IpcServer> endpointPointer(endpoint);
	const QPointer<QProcess> process(worker.process);
	connect(worker.process, &QProcess::started, this, [endpointPointer, process, credentialsLine]() {
		if (endpointPointer && process)
		{
			endpointPointer->setExpectedProcessId(process->processId());
			process->write(credentialsLine->constData(), credentialsLine->size());
		}
		credentialsLine->clear();
		if (process)
		{
			process->closeWriteChannel();
		}
	});

	const QStringList arguments{featureUid.toString(), IpcServer::workerEndpointDirectory()};

	vDebug() << "Starting managed system worker for feature" << VeyonCore::featureManager().feature(featureUid).name();
	if (qEnvironmentVariableIsSet("VEYON_VALGRIND_WORKERS"))
	{
		worker.process->start(QStringLiteral("valgrind"),
							  QStringList{QStringLiteral("--error-limit=no"),
										  QStringLiteral("--leak-check=full"),
										  QStringLiteral("--show-leak-kinds=all"),
										  QStringLiteral("--log-file=valgrind-%1.log").arg(featureUid.toString(QUuid::WithoutBraces)),
										  VeyonCore::filesystem().workerFilePath()} +
								  arguments);
	}
	else
	{
		worker.process->start(VeyonCore::filesystem().workerFilePath(), arguments);
	}

	m_workersMutex.lock();
	m_workers[featureUid] = worker;
	m_workersMutex.unlock();

	watchWorkerAuthentication(featureUid, endpointName);

	return true;
}



bool FeatureWorkerManager::startUnmanagedSessionWorker(Feature::Uid featureUid)
{
	if (thread() != QThread::currentThread())
	{
		vCritical() << "thread mismatch for feature" << featureUid;
		return false;
	}

	stopWorker(featureUid);

	vDebug() << "Starting worker (unmanaged session process) for feature" << featureUid;

	const auto currentUser = VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName);
	if (currentUser.isEmpty())
	{
		vDebug() << "could not determine current user - probably a console session with logon screen";
		return false;
	}

	auto endpoint = createWorkerEndpoint(featureUid, WorkerType::UnmanagedSessionWorker, currentUser);
	if (endpoint == nullptr)
	{
		return false;
	}

	Worker worker;
	worker.endpoint = endpoint;
	worker.endpointName = endpoint->endpointName();

	// runProgramAsUser() reports no PID: the token handshake and the uid/session check are the
	// authenticators of unmanaged workers (02 §6a)
	auto stdInData = endpoint->credentialsLine().toUnprotectedByteArray();
	const auto ret = VeyonCore::platform().coreFunctions().runProgramAsUser(
		VeyonCore::filesystem().workerFilePath(), {featureUid.toString(), IpcServer::workerEndpointDirectory()},
		currentUser, VeyonCore::platform().coreFunctions().activeDesktopName(), stdInData);
	stdInData.fill(0);

	if (ret == false)
	{
		vWarning() << "failed to start worker for feature" << featureUid;
		delete endpoint;
		return false;
	}

	m_workersMutex.lock();
	m_workers[featureUid] = worker;
	m_workersMutex.unlock();

	watchWorkerAuthentication(featureUid, worker.endpointName);

	return true;
}



bool FeatureWorkerManager::stopWorker(Feature::Uid featureUid)
{
	if (thread() != QThread::currentThread())
	{
		vCritical() << "thread mismatch for feature" << featureUid;
		return false;
	}

	QMutexLocker locker(&m_workersMutex);

	if (m_workers.contains(featureUid))
	{
		vDebug() << "Stopping worker for feature" << featureUid;

		auto worker = m_workers.take(featureUid);

		// the endpoint owns the connection: closing it makes an unmanaged worker quit on its own
		if (worker.endpoint)
		{
			worker.endpoint->disconnect(this);
			worker.endpoint->deleteLater();
		}

		if (worker.process)
		{
			auto killTimer = new QTimer;
			connect(killTimer, &QTimer::timeout, worker.process, &QProcess::terminate);
			connect(killTimer, &QTimer::timeout, worker.process, &QProcess::kill);
			connect(killTimer, &QTimer::timeout, killTimer, &QTimer::deleteLater);
			killTimer->start(5000);
		}
	}

	return false;
}



void FeatureWorkerManager::sendMessageToManagedSystemWorker(const FeatureMessage& message)
{
	if (isWorkerRunning(message.featureUid()) == false &&
		startManagedSystemWorker(message.featureUid()) == false)
	{
		vCritical() << "could not start managed system worker";
		return;
	}

	sendMessage(message);
}



void FeatureWorkerManager::sendMessageToUnmanagedSessionWorker(const FeatureMessage& message)
{
	// maintain the order: as long as there are older messages waiting, add them to the list
	if (m_pendingSessionWorkerMessages.contains(message.featureUid()) ||
		(isWorkerRunning(message.featureUid()) == false &&
		 startUnmanagedSessionWorker(message.featureUid()) == false))
	{
		enqueueSessionWorkerMessage(message);
		return;
	}

	sendMessage(message);
}



void FeatureWorkerManager::enqueueSessionWorkerMessage(const FeatureMessage& message)
{
	if (m_pendingSessionWorkerMessages.contains(message.featureUid()) == false)
	{
		vWarning() << "message queue does not exist for feature" << message.featureUid();
		return;
	}

	auto& queue = m_pendingSessionWorkerMessages[message.featureUid()];
	if (queue.size() >= MaximumPendingSessionWorkerMessages)
	{
		// keep the most recent state (e.g. the last tooltip/mode) rather than the oldest messages
		vWarning() << "too many messages pending for session worker of feature" << message.featureUid()
				   << "- discarding the oldest one";
		queue.removeFirst();
	}

	if (queue.isEmpty())
	{
		vDebug() << "User session likely not yet available - retrying worker start";
	}

	queue.append(message);

	if (m_sessionWorkerRetryTimer.isActive() == false)
	{
		m_sessionWorkerRetryTimer.start();
	}
}



void FeatureWorkerManager::retryPendingSessionWorkerMessages()
{
	for (auto it = m_pendingSessionWorkerMessages.begin(); it != m_pendingSessionWorkerMessages.end();)
	{
		const auto featureUid = it.key();
		if (isWorkerRunning(featureUid) || startUnmanagedSessionWorker(featureUid))
		{
			for (const auto& message : std::as_const(it.value()))
			{
				sendMessage(message);
			}
			it = m_pendingSessionWorkerMessages.erase(it);
		}
		else
		{
			++it;
		}
	}

	if (m_pendingSessionWorkerMessages.isEmpty())
	{
		m_sessionWorkerRetryTimer.stop();
	}
}



void FeatureWorkerManager::watchWorkerAuthentication(Feature::Uid featureUid, const QString& endpointName)
{
	// a worker that was launched but never authenticated (credentials not read, process terminated, etc.)
	// would block the feature indefinitely: we remove it so that a subsequent message can restart it
	QTimer::singleShot(WorkerAuthenticationTimeout, this, [=, this]() {
		QMutexLocker locker(&m_workersMutex);
		const auto it = m_workers.constFind(featureUid);
		if (it != m_workers.constEnd() && it->endpointName == endpointName && it->connection.isNull())
		{
			locker.unlock();
			vWarning() << "worker for feature" << featureUid << "did not authenticate in time - stopping it";
			stopWorker(featureUid);
		}
	});
}



void FeatureWorkerManager::removeStaleWorker(Feature::Uid featureUid, const QString& endpointName)
{
	QMutexLocker locker(&m_workersMutex);
	const auto it = m_workers.find(featureUid);
	if (it != m_workers.end() && it->endpointName == endpointName && it->connection.isNull())
	{
		vDebug() << "removing stale worker for feature" << featureUid;
		if (it->endpoint)
		{
			it->endpoint->deleteLater();
		}
		m_workers.erase(it);
	}
}



bool FeatureWorkerManager::isWorkerRunning(Feature::Uid featureUid)
{
	QMutexLocker locker(&m_workersMutex);
	return m_workers.contains(featureUid);
}



IpcServer* FeatureWorkerManager::createWorkerEndpoint(Feature::Uid featureUid, WorkerType workerType,
													  const QString& sessionUser)
{
	const auto directory = IpcServer::workerEndpointDirectory();
	if (IpcServer::prepareWorkerEndpointDirectory(directory) == false)
	{
		vCritical() << "no trusted endpoint directory for workers - not starting worker for feature" << featureUid;
		return nullptr;
	}

	IpcServer::PeerPolicy peerPolicy;
	// a failed handshake stops that worker only, the session server keeps running (02 §6a)
	peerPolicy.maximumFailedHandshakes = 1;
	peerPolicy.windowsSessionId = IpcServer::currentWindowsSessionId();

	if (workerType == WorkerType::ManagedSystemWorker)
	{
		// child process with the server's identity
		peerPolicy.userId = IpcServer::currentUserId();
		peerPolicy.checkProcessId = true;
		peerPolicy.windowsAccess = IpcServer::WindowsAccess::SystemOnly;
	}
	else
	{
		const auto identity = VeyonCore::platform().userFunctions().queryUserIdentity(sessionUser);
#ifdef Q_OS_WIN
		if (identity.securityIdentifier.isEmpty())
#else
		if (identity.userId < 0)
#endif
		{
			vCritical() << "could not determine identity of session user - not starting worker for feature" << featureUid;
			return nullptr;
		}

		peerPolicy.userId = identity.userId;
		// a root server hands the socket over to the session user
		if (identity.userId != IpcServer::currentUserId())
		{
			peerPolicy.socketOwnerUserId = identity.userId;
		}
		peerPolicy.checkProcessId = false;
		peerPolicy.windowsAccess = IpcServer::WindowsAccess::SystemAndUser;
		peerPolicy.windowsUserSid = identity.securityIdentifier;
	}

	auto endpoint = new IpcServer(IpcProtocol::Role::Worker, quint32(qMax(0, VeyonCore::sessionId())), this);
	if (VeyonCore::sessionId() < 0 || endpoint->listen(directory, peerPolicy) == false)
	{
		vCritical() << "failed to create worker endpoint for feature" << featureUid;
		delete endpoint;
		return nullptr;
	}

	const auto endpointName = endpoint->endpointName();

	connect(endpoint, &IpcServer::connectionEstablished, this, [=, this](IpcConnection* connection) {
		handleWorkerConnection(featureUid, endpointName, connection);
	});

	connect(endpoint, &IpcServer::failed, this, [=, this]() {
		vWarning() << "handshake with worker for feature" << featureUid << "failed - stopping worker";
		QMutexLocker locker(&m_workersMutex);
		const auto it = m_workers.constFind(featureUid);
		if (it != m_workers.constEnd() && it->endpointName == endpointName)
		{
			locker.unlock();
			stopWorker(featureUid);
		}
	});

	return endpoint;
}



void FeatureWorkerManager::handleWorkerConnection(Feature::Uid featureUid, const QString& endpointName,
												  IpcConnection* connection)
{
	QMutexLocker locker(&m_workersMutex);

	const auto it = m_workers.find(featureUid);
	if (it == m_workers.end() || it->endpointName != endpointName)
	{
		connection->close();
		return;
	}

	vDebug() << "worker for feature" << featureUid << "connected";

	it->connection = connection;

	connect(connection, &IpcConnection::messageReceived, this, [=, this](const QByteArray& data) {
		handleWorkerMessage(featureUid, data);
	});

	connect(connection, &IpcConnection::closed, this, [=, this]() {
		vDebug() << "connection to worker for feature" << featureUid << "closed";
		removeWorker(featureUid, endpointName);
	});

	sendPendingMessages();
}



void FeatureWorkerManager::handleWorkerMessage(Feature::Uid featureUid, const QByteArray& data)
{
	FeatureMessage message;
	if (message.fromByteArray(data) == false || message.featureUid() != featureUid)
	{
		vCritical() << "invalid message from worker for feature" << featureUid << "- stopping worker";
		stopWorker(featureUid);
		return;
	}

	if (message.command<FeatureMessage::CommandType>() >= 0)
	{
		VeyonCore::featureManager().handleFeatureMessageFromWorker(m_server, message);
	}
}



void FeatureWorkerManager::removeWorker(Feature::Uid featureUid, const QString& endpointName)
{
	QMutexLocker locker(&m_workersMutex);

	const auto it = m_workers.find(featureUid);
	if (it != m_workers.end() && it->endpointName == endpointName)
	{
		vDebug() << "removing worker after its connection has been closed";
		if (it->endpoint)
		{
			it->endpoint->disconnect(this);
			it->endpoint->deleteLater();
		}
		m_workers.erase(it);
	}
}



void FeatureWorkerManager::sendMessage(const FeatureMessage& message)
{
	m_workersMutex.lock();

	if (m_workers.contains(message.featureUid()))
	{
		m_workers[message.featureUid()].pendingMessages.append(message);
	}
	else
	{
		vWarning() << "worker does not exist for feature" << message.featureUid();
	}

	m_workersMutex.unlock();
}



void FeatureWorkerManager::sendPendingMessages()
{
	m_workersMutex.lock();

	for (auto it = m_workers.begin(); it != m_workers.end(); ++it)
	{
		auto& worker = it.value();

		while (worker.connection && worker.connection->isEstablished() && worker.pendingMessages.isEmpty() == false)
		{
			if (worker.connection->sendMessage(worker.pendingMessages.first().toByteArray()) == false)
			{
				vWarning() << "dropping message for worker of feature" << it.key() << "which could not be sent";
			}
			worker.pendingMessages.removeFirst();
		}
	}

	m_workersMutex.unlock();
}
