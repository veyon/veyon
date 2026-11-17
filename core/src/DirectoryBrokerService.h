// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QThread>

#include "DirectoryBrokerLimits.h"
#include "DirectoryBrokerMessage.h"
#include "DirectoryBrokerRateLimiter.h"
#include "IpcChannel.h"

#include <atomic>
#include <functional>
#include <map>
#include <memory>

class DirectoryBrokerWorker;
class NetworkObjectDirectory;
class UserGroupsBackendInterface;

// Service side of the directory and user-groups broker (docs/specs/04-directory-broker.md §5, §6),
// run by veyon-service for its session servers. The directory and the user-groups backend are
// created and used in a dedicated directory worker thread only. Requests are queued per client
// (session server) and executed round-robin, the session servers' own requests (access control)
// before the requests they forward for masters, so that neither another session nor the masters
// of the same session can delay access control decisions. Concurrent directory updates are
// answered by one update. Own and forwarded requests are limited separately in rate, outstanding
// requests, cursors and cursor memory; replies are paged and never contain secrets, replies to
// forwarded requests no directory addresses (DNs).
//
// All functions have to be called in the thread of this object, which is also the thread the
// reply handlers are called in.
class VEYON_CORE_EXPORT DirectoryBrokerService : public QObject
{
	Q_OBJECT
public:
	using Clock = std::function<qint64()>;
	using Request = DirectoryBrokerMessage::Request;
	using Reply = DirectoryBrokerMessage::Reply;
	using ReplyHandler = std::function<void(const Reply&)>;
	// called in the directory worker thread on first use; nullptr makes all requests of that kind
	// fail with "unavailable" until the factory is tried again after BackendRetryInterval. The
	// service takes ownership of the directory, not of the backend.
	using DirectoryFactory = std::function<NetworkObjectDirectory*()>;
	using UserGroupsBackendFactory = std::function<UserGroupsBackendInterface*()>;

	DirectoryBrokerService(const DirectoryFactory& directoryFactory,
						   const UserGroupsBackendFactory& userGroupsBackendFactory,
						   const Clock& clock = IpcChannel::steadyNow,
						   QObject* parent = nullptr);
	~DirectoryBrokerService() override;

	void setRequestTimeout(int timeout)
	{
		m_requestTimeout = timeout;
	}

	// tests
	void setBackendRetryInterval(int interval);
	void setForwardedExecutionBudget(int budget);

	// replyHandler is called exactly once unless the client is removed before
	void handleRequest(const QString& clientId, const Request& request, const ReplyHandler& replyHandler);

	void removeClient(const QString& clientId);

	// for backends running in the directory worker thread (cooperative abort, e.g. between LDAP
	// searches): whether the request being executed in the calling thread has timed out, exceeded
	// its execution budget (forwarded requests) or its client is gone; always false in other threads
	static bool isCurrentRequestCancelled();

private:
	struct Cursor
	{
		QString command;
		QVariantMap extraArguments;
		QVariantList items;
		QList<int> itemSizes;
		qint64 size = 0;
		qint64 expiryTime = 0;
		quint64 serial = 0;
	};

	struct CursorPool
	{
		int maximumCursors;
		qint64 maximumMemory;
		std::map<QString, Cursor> cursors{};
		qint64 memory = 0;
	};

	struct Client
	{
		DirectoryBrokerRateLimiter ownLimiter{DirectoryBrokerLimits::MaximumRequestsPerSecond,
											  DirectoryBrokerLimits::MaximumOutstandingRequests};
		DirectoryBrokerRateLimiter forwardedLimiter{DirectoryBrokerLimits::MaximumForwardedRequestsPerSecond,
													DirectoryBrokerLimits::MaximumForwardedOutstandingRequests};
		CursorPool ownCursors{DirectoryBrokerLimits::MaximumOwnCursors, DirectoryBrokerLimits::MaximumOwnCursorMemory};
		CursorPool forwardedCursors{DirectoryBrokerLimits::MaximumForwardedCursors,
									DirectoryBrokerLimits::MaximumForwardedCursorMemory};

		DirectoryBrokerRateLimiter& limiter(bool forwarded)
		{
			return forwarded ? forwardedLimiter : ownLimiter;
		}

		CursorPool& cursorPool(bool forwarded)
		{
			return forwarded ? forwardedCursors : ownCursors;
		}
	};

	struct PendingRequest
	{
		QString clientId;
		Request request;
		ReplyHandler replyHandler;
		std::shared_ptr<std::atomic<bool>> cancelled;
	};

	void continueWithCursor(CursorPool& pool, const Request& request, const ReplyHandler& replyHandler);
	void finishRequest(quint64 jobId, const Reply& result);
	void timeoutRequest(quint64 jobId);

	Reply createPagedReply(CursorPool& pool, const Request& request, const Reply& result);
	Reply nextPage(CursorPool& pool, const QString& cursorId, Cursor& cursor, quint32 requestId);
	void removeExpiredCursors(CursorPool& pool);
	void removeCursor(CursorPool& pool, const QString& cursorId);

	static int encodedSize(const QVariant& item);

	const Clock m_clock;
	int m_requestTimeout = DirectoryBrokerLimits::RequestTimeout;

	QThread m_workerThread;
	DirectoryBrokerWorker* m_worker;

	std::map<QString, Client> m_clients;
	std::map<quint64, PendingRequest> m_pendingRequests;
	quint64 m_nextJobId = 1;
	quint64 m_nextCursorSerial = 1;
};
