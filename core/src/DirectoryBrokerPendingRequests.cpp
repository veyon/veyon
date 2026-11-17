// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DirectoryBrokerPendingRequests.h"


quint32 DirectoryBrokerPendingRequests::add(const QString& command, const ReplyHandler& handler)
{
	QMutexLocker locker(&m_mutex);

	const auto requestId = m_nextRequestId++;
	m_requests[requestId] = PendingRequest{command, handler, std::nullopt, false};

	return requestId;
}



bool DirectoryBrokerPendingRequests::complete(const Reply& reply, ReplyHandler& handler)
{
	QMutexLocker locker(&m_mutex);

	const auto it = m_requests.find(reply.requestId);
	if (it == m_requests.end() || it->second.reply.has_value() || it->second.failed)
	{
		vDebug() << "discarding unknown, late or duplicate reply" << reply.requestId;
		return false;
	}

	if (reply.isError() == false && reply.command != it->second.command)
	{
		vWarning() << "discarding reply" << reply.command << "for request" << it->second.command;
		return false;
	}

	if (it->second.handler)
	{
		handler = it->second.handler;
		m_requests.erase(it);
		return true;
	}

	it->second.reply = reply;
	m_replyReceived.wakeAll();

	return true;
}



std::optional<DirectoryBrokerPendingRequests::Reply> DirectoryBrokerPendingRequests::wait(quint32 requestId, QDeadlineTimer deadline)
{
	QMutexLocker locker(&m_mutex);

	auto it = m_requests.find(requestId);
	while (it != m_requests.end() && it->second.reply.has_value() == false && it->second.failed == false)
	{
		if (m_replyReceived.wait(&m_mutex, deadline) == false)
		{
			break;
		}
		it = m_requests.find(requestId);
	}

	if (it == m_requests.end())
	{
		return {};
	}

	const auto reply = it->second.reply;
	m_requests.erase(it);

	return reply;
}



std::optional<DirectoryBrokerPendingRequests::ReplyHandler> DirectoryBrokerPendingRequests::remove(quint32 requestId)
{
	QMutexLocker locker(&m_mutex);

	const auto it = m_requests.find(requestId);
	if (it == m_requests.end())
	{
		return {};
	}

	const auto handler = it->second.handler;
	if (handler)
	{
		m_requests.erase(it);
		return handler;
	}

	// a synchronous waiter removes the request itself
	it->second.failed = true;
	m_replyReceived.wakeAll();

	return {};
}



QList<DirectoryBrokerPendingRequests::ReplyHandler> DirectoryBrokerPendingRequests::removeAll()
{
	QMutexLocker locker(&m_mutex);

	QList<ReplyHandler> handlers;

	for (auto it = m_requests.begin(); it != m_requests.end();)
	{
		if (it->second.handler)
		{
			handlers.append(it->second.handler);
			it = m_requests.erase(it);
		}
		else
		{
			it->second.failed = true;
			++it;
		}
	}

	m_replyReceived.wakeAll();

	return handlers;
}



int DirectoryBrokerPendingRequests::count() const
{
	QMutexLocker locker(&m_mutex);
	return int(m_requests.size());
}
