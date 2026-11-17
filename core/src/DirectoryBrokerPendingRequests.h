// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDeadlineTimer>
#include <QMutex>
#include <QWaitCondition>

#include "DirectoryBrokerMessage.h"

#include <functional>
#include <map>
#include <optional>

// Outstanding requests of one broker client connection (docs/specs/04-directory-broker.md §4):
// assigns monotonically increasing RequestIds and matches replies to them. Replies with an unknown
// RequestId, a second reply for the same request, replies arriving after the request was given up
// (timeout) and replies whose command does not match the request are discarded. Thread-safe.
class VEYON_CORE_EXPORT DirectoryBrokerPendingRequests
{
public:
	using Reply = DirectoryBrokerMessage::Reply;
	using ReplyHandler = std::function<void(const Reply&)>;

	// without handler, wait() has to be called for the request
	quint32 add(const QString& command, const ReplyHandler& handler = {});

	// returns false if the reply is discarded; for asynchronous requests the handler is returned
	// in handler and has to be called by the caller (outside of any lock)
	bool complete(const Reply& reply, ReplyHandler& handler);

	// waits for the reply of a synchronous request and removes the request in any case
	std::optional<Reply> wait(quint32 requestId, QDeadlineTimer deadline);

	// removes a request (e.g. after its timeout) and returns its handler if it was still pending
	std::optional<ReplyHandler> remove(quint32 requestId);

	// removes all asynchronous requests and returns their handlers, wakes all synchronous waiters
	// (which then fail)
	QList<ReplyHandler> removeAll();

	int count() const;

private:
	struct PendingRequest
	{
		QString command;
		ReplyHandler handler;
		std::optional<Reply> reply;
		bool failed = false;
	};

	mutable QMutex m_mutex;
	QWaitCondition m_replyReceived;
	std::map<quint32, PendingRequest> m_requests;
	quint32 m_nextRequestId = 1;
};
