// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-FileCopyrightText: 2026 Ajrat Makhmutov <rauty@altlinux.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDBusPendingCallWatcher>

#include <algorithm>

#include "PortalInputDispatcher.h"
#include "VeyonCore.h"


PortalInputDispatcher::PortalInputDispatcher(QObject* parent) :
	QObject(parent),
	m_replyTimer(this)
{
	m_replyTimer.setSingleShot(true);
	m_replyTimer.setInterval(DefaultReplyTimeout);
	connect(&m_replyTimer, &QTimer::timeout, this, [this]() {
		const auto completionHandler = m_currentCompletionHandler;
		abandonCurrentRequest();
		handleTimeout(QDBusError(QDBusError::NoReply, QStringLiteral("no reply to portal input request")),
					  completionHandler);
	});
}



PortalInputDispatcher::~PortalInputDispatcher()
{
	clear();
}



void PortalInputDispatcher::enqueue(RequestType type, const Request& request,
									const CompletionHandler& completionHandler)
{
	if (m_failed)
	{
		return;
	}

	// only the latest position matters, and nothing pending may depend on an earlier one
	if (type == RequestType::PointerMotion && m_pendingRequests.isEmpty() == false &&
		m_pendingRequests.last().type == RequestType::PointerMotion)
	{
		m_pendingRequests.last().request = request;
		m_pendingRequests.last().completionHandler = completionHandler;
		return;
	}

	if (m_pendingRequests.size() >= MaximumPendingRequests)
	{
		fail(QDBusError(QDBusError::LimitsExceeded, QStringLiteral("too many pending portal input requests")));
		return;
	}

	m_pendingRequests.enqueue({type, request, completionHandler});
	sendNextRequest();
}



void PortalInputDispatcher::clear()
{
	m_pendingRequests.clear();
	abandonCurrentRequest();
}



void PortalInputDispatcher::abandonCurrentRequest()
{
	m_replyTimer.stop();
	m_currentCompletionHandler = {};

	if (m_currentRequest)
	{
		m_currentRequest->disconnect(this);
		m_currentRequest->deleteLater();
		m_currentRequest = nullptr;
	}
}



void PortalInputDispatcher::reset()
{
	clear();
	m_failed = false;
	m_consecutiveErrors = 0;
	m_consecutiveTimeouts = 0;
	m_lastErrorReport.invalidate();
	m_suppressedErrorReports = 0;
}



bool PortalInputDispatcher::isPortalUnavailable(const QDBusError& error)
{
	switch (error.type())
	{
	case QDBusError::ServiceUnknown:
	case QDBusError::Disconnected:
	case QDBusError::NoServer:
	case QDBusError::NoNetwork:
	case QDBusError::UnknownObject:
	case QDBusError::UnknownInterface:
	case QDBusError::UnknownMethod: return true;
	default: break;
	}

	return false;
}



bool PortalInputDispatcher::isTimeout(const QDBusError& error)
{
	switch (error.type())
	{
	case QDBusError::NoReply:
	case QDBusError::Timeout:
	case QDBusError::TimedOut: return true;
	default: break;
	}

	return false;
}



bool PortalInputDispatcher::isRejectedPosition(RequestType type, const QDBusError& error)
{
	// xdg-desktop-portal replies with org.freedesktop.DBus.Error.Failed "Invalid position" to
	// positions outside of the stream, while a closed session is AccessDenied ("Invalid session")
	return type == RequestType::PointerMotion &&
		   (error.type() == QDBusError::Failed || error.type() == QDBusError::InvalidArgs);
}



void PortalInputDispatcher::sendNextRequest()
{
	if (m_currentRequest || m_pendingRequests.isEmpty() || m_failed)
	{
		return;
	}

	const auto pendingRequest = m_pendingRequests.dequeue();

	// the finished() signal is emitted from the event loop even if the call is finished already
	m_currentRequestType = pendingRequest.type;
	m_currentCompletionHandler = pendingRequest.completionHandler;
	m_currentRequest = new QDBusPendingCallWatcher(pendingRequest.request(), this);
	connect(m_currentRequest, &QDBusPendingCallWatcher::finished, this, &PortalInputDispatcher::finishCurrentRequest);

	m_replyTimer.start();
}



void PortalInputDispatcher::finishCurrentRequest()
{
	auto watcher = m_currentRequest;
	if (watcher == nullptr || sender() != watcher)
	{
		return;
	}

	m_replyTimer.stop();
	m_currentRequest = nullptr;
	watcher->deleteLater();

	const auto completionHandler = m_currentCompletionHandler;
	m_currentCompletionHandler = {};

	if (watcher->isError())
	{
		const auto error = watcher->error();

		if (isTimeout(error))
		{
			handleTimeout(error, completionHandler);
			return;
		}

		m_consecutiveTimeouts = 0;

		// every later request would fail as well
		if (isPortalUnavailable(error) ||
			(isRejectedPosition(m_currentRequestType, error) == false &&
			 ++m_consecutiveErrors >= MaximumConsecutiveErrors))
		{
			fail(error);
			return;
		}

		// the request has been dropped, later ones are still valid
		reportDroppedRequest(error);
	}
	else
	{
		m_consecutiveErrors = 0;
		m_consecutiveTimeouts = 0;
	}

	// may enqueue further requests, e.g. to retry a release
	if (completionHandler)
	{
		completionHandler(watcher->isError() ? Result::Rejected : Result::Accepted);
	}

	sendNextRequest();
}



void PortalInputDispatcher::handleTimeout(const QDBusError& error, const CompletionHandler& completionHandler)
{
	// a portal (or compositor) that stalls briefly, e.g. under load, must not restart the whole
	// session including capture and possibly the consent dialog; the input pending in the meantime
	// is outdated though, except for releases, without which keys or buttons would stay pressed,
	// and the last pointer motion, which is the current position
	if (++m_consecutiveTimeouts >= MaximumConsecutiveTimeouts)
	{
		fail(error);
		return;
	}

	const auto pendingCount = pendingRequestCount();
	const auto lastMotion = std::find_if(m_pendingRequests.crbegin(), m_pendingRequests.crend(),
										 [](const PendingRequest& pendingRequest) {
		return pendingRequest.type == RequestType::PointerMotion;
	});
	const auto lastMotionIndex = lastMotion == m_pendingRequests.crend() ?
									 -1 : int(std::distance(lastMotion, m_pendingRequests.crend())) - 1;

	QQueue<PendingRequest> keptRequests;
	for (int i = 0; i < pendingCount; ++i)
	{
		if (m_pendingRequests[i].type == RequestType::Release || i == lastMotionIndex)
		{
			keptRequests.enqueue(m_pendingRequests[i]);
		}
	}
	m_pendingRequests = keptRequests;

	vWarning() << "no reply to portal input request, dropping" << pendingCount - pendingRequestCount() + 1
			   << "pending input requests, keeping" << pendingRequestCount();

	if (completionHandler)
	{
		completionHandler(Result::NoReply);
	}

	sendNextRequest();
}



void PortalInputDispatcher::reportDroppedRequest(const QDBusError& error)
{
	// e.g. a session without access to one device type would log a warning per input event
	if (m_lastErrorReport.isValid() && m_lastErrorReport.elapsed() < ErrorLogInterval)
	{
		++m_suppressedErrorReports;
		vDebug() << "portal input request failed:" << error.name() << error.message();
		return;
	}

	if (m_suppressedErrorReports > 0)
	{
		vWarning() << "portal input request failed:" << error.name() << error.message()
				   << QStringLiteral("(%1 more failed since the last report)").arg(m_suppressedErrorReports);
	}
	else
	{
		vWarning() << "portal input request failed:" << error.name() << error.message();
	}

	m_lastErrorReport.start();
	m_suppressedErrorReports = 0;
}



void PortalInputDispatcher::fail(const QDBusError& error)
{
	clear();
	m_failed = true;

	vWarning() << "portal input failed:" << error.name() << error.message();

	if (m_failureHandler)
	{
		m_failureHandler(error);
	}
}
