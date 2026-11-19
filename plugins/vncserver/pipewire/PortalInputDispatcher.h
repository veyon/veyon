// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-FileCopyrightText: 2026 Ajrat Makhmutov <rauty@altlinux.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDBusError>
#include <QDBusPendingCall>
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
 * Error replies (e.g. a device type the session has no access to) only drop the affected request.
 * If the portal does not reply in time or too many requests are pending, the dispatcher fails:
 * it discards all requests, calls the failure handler and ignores new requests until reset().
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
	static constexpr int DefaultReplyTimeout = 5000;

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
	void fail(const QDBusError& error);

	QQueue<PendingRequest> m_pendingRequests;
	QDBusPendingCallWatcher* m_currentRequest = nullptr;
	QTimer m_replyTimer;
	FailureHandler m_failureHandler;
	bool m_failed = false;
	bool m_errorReported = false;
};
