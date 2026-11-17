// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDeadlineTimer>
#include <QHostAddress>
#include <QThread>
#include <QTimer>

#include "ComputerControlInterface.h"
#include "DirectoryBrokerConnectionClient.h"
#include "DirectoryBrokerLimits.h"
#include "FeatureMessage.h"
#include "VeyonConfiguration.h"
#include "VeyonConnection.h"


using namespace DirectoryBrokerProtocol;


DirectoryBrokerConnectionClient::DirectoryBrokerConnectionClient(const QString& host, QObject* parent) :
	QObject(parent),
	m_host(host),
	m_isLocalHost(isLocalHost(host))
{
	if (m_isLocalHost == false)
	{
		vCritical() << "DirectoryBroker/Host" << host << "is not the local computer - a central directory broker "
														 "requires a verified TLS connection, refusing all directory broker requests";
	}
}



DirectoryBrokerConnectionClient::~DirectoryBrokerConnectionClient()
{
	(void)m_pendingRequests.removeAll();

	m_stateMutex.lock();
	m_connection = nullptr;
	m_stateMutex.unlock();

	delete m_computerControlInterface;
}



DirectoryBrokerClient::Reply DirectoryBrokerConnectionClient::request(const QString& command, const QVariantMap& arguments)
{
	if (m_isLocalHost == false)
	{
		return DirectoryBrokerMessage::error(0, Error::Unavailable, tr("directory host identity not verified"));
	}

	const QDeadlineTimer deadline(DirectoryBrokerLimits::RequestTimeout);

	if (QThread::currentThread() == thread())
	{
		start();
	}
	else
	{
		QMetaObject::invokeMethod(this, [this]() { start(); }, Qt::QueuedConnection);
	}

	m_stateMutex.lock();
	while (m_state != VncConnection::State::Connected && isFailedState(m_state) == false)
	{
		if (m_stateChanged.wait(&m_stateMutex, deadline) == false)
		{
			break;
		}
	}
	const auto state = m_state;
	const auto connection = m_connection;
	m_stateMutex.unlock();

	if (state != VncConnection::State::Connected || connection == nullptr)
	{
		const auto message = stateErrorMessage(state);
		vWarning() << "directory broker not available:" << message;
		return DirectoryBrokerMessage::error(0, Error::Unavailable, message);
	}

	for (;;)
	{
		const auto requestId = m_pendingRequests.add(command);

		FeatureMessage message{DirectoryBrokerMessage::featureUid()};
		DirectoryBrokerMessage::toFeatureMessage(DirectoryBrokerMessage::Request{requestId, command, arguments}, message);
		connection->sendFeatureMessage(message);

		const auto reply = m_pendingRequests.wait(requestId, deadline);
		if (reply.has_value() == false)
		{
			vWarning() << "no reply from directory broker for" << command;
			return DirectoryBrokerMessage::error(requestId, Error::Timeout, tr("no reply from the directory broker"));
		}

		// bursts (many pages, MAC address lookups for many computers) exceed the request rate of
		// the connection: retry within the request's deadline
		if (reply->errorCode() == Error::RateLimited && deadline.remainingTime() > RateLimitRetryDelay)
		{
			QThread::msleep(RateLimitRetryDelay);
			continue;
		}

		return *reply;
	}
}



void DirectoryBrokerConnectionClient::requestAsync(const QString& command, const QVariantMap& arguments, const ReplyHandler& handler)
{
	if (m_isLocalHost == false)
	{
		QMetaObject::invokeMethod(this, [handler]() {
			handler(DirectoryBrokerMessage::error(0, Error::Unavailable, tr("directory host identity not verified")));
		}, Qt::QueuedConnection);
		return;
	}

	sendAsyncRequest(command, arguments, handler, QDeadlineTimer(DirectoryBrokerLimits::RequestTimeout));
}



void DirectoryBrokerConnectionClient::sendAsyncRequest(const QString& command, const QVariantMap& arguments,
													   const ReplyHandler& handler, QDeadlineTimer deadline)
{
	const auto requestId = m_pendingRequests.add(command, [=, this](const Reply& reply) {
		// see request()
		if (reply.errorCode() == Error::RateLimited && deadline.remainingTime() > RateLimitRetryDelay)
		{
			QTimer::singleShot(RateLimitRetryDelay, this, [=, this]() { sendAsyncRequest(command, arguments, handler, deadline); });
			return;
		}
		handler(reply);
	});

	QTimer::singleShot(int(qMax<qint64>(0, deadline.remainingTime())), this, [this, requestId, command]() {
		const auto timedOutHandler = m_pendingRequests.remove(requestId);
		if (timedOutHandler.has_value())
		{
			vWarning() << "no reply from directory broker for" << command;
			(*timedOutHandler)(DirectoryBrokerMessage::error(requestId, Error::Timeout, tr("no reply from the directory broker")));
		}
	});

	m_queuedRequests.append({requestId, command, arguments});

	start();
	sendQueuedRequests();
}



void DirectoryBrokerConnectionClient::start()
{
	QMutexLocker locker(&m_stateMutex);

	if (m_started)
	{
		return;
	}

	m_started = true;
	locker.unlock();

	// the local session server of this session, as for the master's local session control
	// connect to the loopback address, not to whatever "localhost" resolves to
	const auto connectHost = m_host.compare(QLatin1String("localhost"), Qt::CaseInsensitive) == 0 ? QHostAddress(QHostAddress::LocalHost).toString() : m_host;
	m_computerControlInterface = new ComputerControlInterface(Computer(NetworkObject::Uid::createUuid(),
																	   QStringLiteral("DirectoryBroker"), connectHost),
															  VeyonCore::config().veyonServerPort() + VeyonCore::sessionId(),
															  this);
	m_computerControlInterface->start({}, ComputerControlInterface::UpdateMode::FeatureControlOnly);

	const auto connection = m_computerControlInterface->connection();
	if (connection == nullptr)
	{
		vCritical() << "failed to create connection to directory broker host" << m_host;
		locker.relock();
		m_state = VncConnection::State::ConnectionFailed;
		m_stateChanged.wakeAll();
		return;
	}

	connect(connection, &VeyonConnection::featureMessageReceived,
			this, &DirectoryBrokerConnectionClient::handleFeatureMessage, Qt::DirectConnection);
	connect(connection->vncConnection(), &VncConnection::stateChanged,
			this, &DirectoryBrokerConnectionClient::updateState, Qt::DirectConnection);

	locker.relock();
	m_connection = connection;
	locker.unlock();

	updateState();
}



void DirectoryBrokerConnectionClient::updateState()
{
	m_stateMutex.lock();
	if (m_connection)
	{
		m_state = m_connection->state();
	}
	m_stateChanged.wakeAll();
	m_stateMutex.unlock();

	QMetaObject::invokeMethod(this, [this]() { sendQueuedRequests(); }, Qt::QueuedConnection);
}



void DirectoryBrokerConnectionClient::handleFeatureMessage(const FeatureMessage& message)
{
	if (message.featureUid() != DirectoryBrokerMessage::featureUid())
	{
		return;
	}

	const auto reply = DirectoryBrokerMessage::replyFromFeatureMessage(message);
	if (reply.has_value() == false)
	{
		vWarning() << "discarding malformed directory broker reply";
		return;
	}

	ReplyHandler handler;
	if (m_pendingRequests.complete(*reply, handler) && handler)
	{
		const auto completedReply = *reply;
		QMetaObject::invokeMethod(this, [handler, completedReply]() { handler(completedReply); }, Qt::QueuedConnection);
	}
}



void DirectoryBrokerConnectionClient::sendQueuedRequests()
{
	if (m_queuedRequests.isEmpty())
	{
		return;
	}

	m_stateMutex.lock();
	const auto state = m_state;
	const auto connection = m_connection;
	m_stateMutex.unlock();

	if (state == VncConnection::State::Connected && connection)
	{
		const auto requests = m_queuedRequests;
		m_queuedRequests.clear();
		for (const auto& request : requests)
		{
			FeatureMessage message{DirectoryBrokerMessage::featureUid()};
			DirectoryBrokerMessage::toFeatureMessage(DirectoryBrokerMessage::Request{request.requestId, request.command, request.arguments},
													 message);
			connection->sendFeatureMessage(message);
		}
	}
	else if (isFailedState(state))
	{
		const auto requests = m_queuedRequests;
		m_queuedRequests.clear();
		const auto message = stateErrorMessage(state);
		vWarning() << "directory broker not available:" << message;
		for (const auto& request : requests)
		{
			const auto handler = m_pendingRequests.remove(request.requestId);
			if (handler.has_value())
			{
				(*handler)(DirectoryBrokerMessage::error(request.requestId, Error::Unavailable, message));
			}
		}
	}
}



bool DirectoryBrokerConnectionClient::isFailedState(VncConnection::State state)
{
	switch (state)
	{
	case VncConnection::State::HostOffline:
	case VncConnection::State::HostNameResolutionFailed:
	case VncConnection::State::ServerNotRunning:
	case VncConnection::State::AuthenticationFailed:
	case VncConnection::State::AccessControlFailed:
	case VncConnection::State::ConnectionFailed:
		return true;
	default:
		break;
	}

	return false;
}



QString DirectoryBrokerConnectionClient::stateErrorMessage(VncConnection::State state)
{
	switch (state)
	{
	case VncConnection::State::ServerNotRunning:
	case VncConnection::State::HostOffline:
		return tr("the Veyon Server of this computer is not running");
	case VncConnection::State::AuthenticationFailed:
		return tr("authentication at the Veyon Server of this computer failed");
	case VncConnection::State::AccessControlFailed:
		return tr("access to the Veyon Server of this computer denied by access control");
	case VncConnection::State::Connected:
		return {};
	default:
		break;
	}

	return tr("no connection to the Veyon Server of this computer");
}
