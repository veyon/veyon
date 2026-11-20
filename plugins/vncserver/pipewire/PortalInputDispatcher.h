// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-FileCopyrightText: 2026 Ajrat Makhmutov <rauty@altlinux.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDBusError>
#include <QDBusPendingCall>
#include <QElapsedTimer>
#include <QObject>
#include <QQueue>
#include <QTimer>

#include <functional>

class QDBusPendingCallWatcher;

/**
 * @brief Sends input requests (Notify* calls) to the XDG Desktop Portal one after another.
 *
 * xdg-desktop-portal handles each method call in a thread of its own, so requests sent without
 * waiting for the reply to the previous one can reach the compositor in a different order, e.g.
 * a key before the modifier pressed in front of it. The portal replies once it has forwarded a
 * request, so the next request is sent only after the reply to the current one.
 *
 * Error replies (e.g. a device type the session has no access to or a pointer position outside
 * of the stream) only drop the affected request and are logged rate-limited. If the portal does
 * not reply in time, the request and all pending ones are dropped, as the input would be outdated
 * once the portal catches up.
 *
 * The dispatcher fails if the portal is unavailable (transport errors such as ServiceUnknown or
 * Disconnected), after MaximumConsecutiveErrors error replies in a row (e.g. once the session has
 * been closed), if MaximumConsecutiveTimeouts requests in a row are not replied to in time or if
 * too many requests are pending: it discards all requests, calls the failure handler and ignores
 * new requests until reset(). Error replies to pointer motions which only reject the position do
 * not count as errors in a row, so a client can't make the session fail by pointer input alone.
 */
class PortalInputDispatcher : public QObject
{
	Q_OBJECT
public:
	using Request = std::function<QDBusPendingCall()>;
	using FailureHandler = std::function<void(const QDBusError& error)>;

	enum class RequestType {
		Default,
		// replaces a pointer motion that is still pending, i.e. not sent yet
		PointerMotion,
	};

	static constexpr int MaximumPendingRequests = 4096;
	static constexpr int MaximumConsecutiveErrors = 8;
	static constexpr int MaximumConsecutiveTimeouts = 2;
	static constexpr int DefaultReplyTimeout = 5000;
	static constexpr int ErrorLogInterval = 10000;

	explicit PortalInputDispatcher(QObject* parent = nullptr);
	~PortalInputDispatcher() override;

	void setFailureHandler(const FailureHandler& failureHandler)
	{
		m_failureHandler = failureHandler;
	}

	void setReplyTimeout(int milliseconds)
	{
		m_replyTimer.setInterval(milliseconds);
	}

	void enqueue(RequestType type, const Request& request);

	// discards all pending requests and stops waiting for the reply to the current one
	void clear();

	// clear() and accept requests again after a failure
	void reset();

	bool hasFailed() const
	{
		return m_failed;
	}

	int pendingRequestCount() const
	{
		return int(m_pendingRequests.size());
	}

private:
	struct PendingRequest
	{
		RequestType type;
		Request request;
	};

	void sendNextRequest();
	void finishCurrentRequest();
	void handleTimeout(const QDBusError& error);
	void reportDroppedRequest(const QDBusError& error);
	void fail(const QDBusError& error);

	static bool isPortalUnavailable(const QDBusError& error);
	static bool isTimeout(const QDBusError& error);
	static bool isRejectedPosition(RequestType type, const QDBusError& error);

	QQueue<PendingRequest> m_pendingRequests;
	QDBusPendingCallWatcher* m_currentRequest = nullptr;
	RequestType m_currentRequestType = RequestType::Default;
	QTimer m_replyTimer;
	FailureHandler m_failureHandler;
	bool m_failed = false;
	int m_consecutiveErrors = 0;
	int m_consecutiveTimeouts = 0;
	QElapsedTimer m_lastErrorReport;
	int m_suppressedErrorReports = 0;
};
