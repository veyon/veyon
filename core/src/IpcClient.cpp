// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDeadlineTimer>
#include <QLocalSocket>
#include <QTimer>

#include "IpcClient.h"
#include "IpcConnection.h"
#include "IpcServer.h"


IpcClient::IpcClient(IpcProtocol::Role role, quint32 sessionId, QObject* parent) :
	QObject(parent),
	m_role(role),
	m_sessionId(sessionId)
{
}



IpcClient::~IpcClient()
{
	m_asyncCalls.clear();
	delete m_connection;
}



bool IpcClient::connectAndHandshake(const QString& directory, IpcEndpointCredentials&& credentials, int timeout)
{
	if (m_connection)
	{
		vCritical() << "already connected";
		return false;
	}

	QDeadlineTimer deadline(timeout);

	const auto endpointName = credentials.endpointName();
	auto socket = IpcConnection::connectToEndpoint(directory, endpointName, timeout);
	if (socket == nullptr)
	{
		return false;
	}

	m_connection = new IpcConnection(socket, IpcChannel::Side::Client, m_role, m_sessionId,
									 std::move(credentials), nullptr, this);
	connect(m_connection, &IpcConnection::messageReceived, this, &IpcClient::handleMessage);
	connect(m_connection, &IpcConnection::closed, this, [this]() {
		vWarning() << "IPC connection closed";
		failAsyncCalls();
	});

	m_connection->start();

	if (m_connection->waitForEstablished(int(qMax<qint64>(0, deadline.remainingTime()))) == false)
	{
		vCritical() << "handshake with" << endpointName << "failed";
		delete m_connection;
		return false;
	}

	return true;
}



bool IpcClient::isConnected() const
{
	return m_connection && m_connection->isEstablished() && m_connection->isClosed() == false;
}



IpcClient::Response IpcClient::call(QLatin1String command, const QJsonObject& arguments, int timeout)
{
	if (isConnected() == false)
	{
		return {};
	}

	const auto requestId = m_nextRequestId++;
	m_receivedMessages.clear();

	if (m_connection->sendMessage(IpcMessage::encodeRequest({requestId, command, arguments})) == false)
	{
		vCritical() << "failed to send request" << command;
		return {};
	}

	QDeadlineTimer deadline(timeout);
	while (deadline.hasExpired() == false)
	{
		while (m_receivedMessages.isEmpty() == false)
		{
			const auto response = IpcMessage::decodeResponse(m_receivedMessages.takeFirst());
			if (response.has_value() && response->id == requestId)
			{
				return *response;
			}
			vWarning() << "discarding unexpected or malformed response";
		}

		if (m_connection->waitForReadyRead(int(qMax<qint64>(1, deadline.remainingTime()))) == false &&
			m_connection->isClosed())
		{
			break;
		}
	}

	vCritical() << "no response for request" << command;
	return {};
}



void IpcClient::callAsync(const QString& command, const QJsonObject& arguments, const ResponseHandler& handler, int timeout)
{
	const auto fail = [this, handler]() {
		QMetaObject::invokeMethod(this, [handler]() { handler({}); }, Qt::QueuedConnection);
	};

	if (isConnected() == false)
	{
		fail();
		return;
	}

	if (int(m_asyncCalls.size()) >= MaximumAsyncCalls)
	{
		vWarning() << "too many asynchronous calls in flight";
		fail();
		return;
	}

	const auto requestId = m_nextRequestId++;

	if (m_connection->sendMessage(IpcMessage::encodeRequest({requestId, command, arguments})) == false)
	{
		vCritical() << "failed to send request" << command;
		fail();
		return;
	}

	m_asyncCalls[requestId] = handler;

	QTimer::singleShot(timeout, this, [this, requestId, command]() {
		if (m_asyncCalls.find(requestId) != m_asyncCalls.end())
		{
			vCritical() << "no response for request" << command;
			finishAsyncCall(requestId, {});
		}
	});
}



void IpcClient::handleMessage(const QByteArray& message)
{
	if (m_asyncCalls.empty() == false)
	{
		const auto response = IpcMessage::decodeResponse(message);
		if (response.has_value() && m_asyncCalls.find(response->id) != m_asyncCalls.end())
		{
			// deferred, as this may run inside the wait loop of a synchronous call
			const auto requestId = response->id;
			const auto asyncResponse = *response;
			QMetaObject::invokeMethod(this, [this, requestId, asyncResponse]() { finishAsyncCall(requestId, asyncResponse); },
									  Qt::QueuedConnection);
			return;
		}
	}

	// at most one synchronous request is in flight, so only a few stray messages can queue up
	if (m_receivedMessages.size() < IpcProtocol::MaximumInFlightRequests)
	{
		m_receivedMessages.append(message);
	}
}



void IpcClient::finishAsyncCall(int requestId, const Response& response)
{
	const auto it = m_asyncCalls.find(requestId);
	if (it == m_asyncCalls.end())
	{
		// late response after a timeout
		return;
	}

	const auto handler = it->second;
	m_asyncCalls.erase(it);

	handler(response);
}



void IpcClient::failAsyncCalls()
{
	const auto requestIds = [this]() {
		QList<int> ids;
		for (const auto& asyncCall : m_asyncCalls)
		{
			ids.append(asyncCall.first);
		}
		return ids;
	}();

	for (const auto requestId : requestIds)
	{
		QMetaObject::invokeMethod(this, [this, requestId]() { finishAsyncCall(requestId, {}); }, Qt::QueuedConnection);
	}
}
