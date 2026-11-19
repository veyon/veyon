// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-FileCopyrightText: 2026 Ajrat Makhmutov <rauty@altlinux.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDBusPendingCallWatcher>

#include "PortalInputDispatcher.h"
#include "VeyonCore.h"


PortalInputDispatcher::PortalInputDispatcher(QObject* parent) :
	QObject(parent),
	m_replyTimer(this)
{
	m_replyTimer.setSingleShot(true);
	m_replyTimer.setInterval(DefaultReplyTimeout);
	connect(&m_replyTimer, &QTimer::timeout, this, [this]() {
		fail(QDBusError(QDBusError::NoReply, QStringLiteral("no reply to portal input request")));
	});
}



PortalInputDispatcher::~PortalInputDispatcher()
{
	clear();
}



void PortalInputDispatcher::enqueue(RequestType type, const Request& request)
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
		return;
	}

	if (m_pendingRequests.size() >= MaximumPendingRequests)
	{
		fail(QDBusError(QDBusError::LimitsExceeded, QStringLiteral("too many pending portal input requests")));
		return;
	}

	m_pendingRequests.enqueue({type, request});
	sendNextRequest();
}



void PortalInputDispatcher::clear()
{
	m_pendingRequests.clear();
	m_replyTimer.stop();

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
	m_errorReported = false;
}



void PortalInputDispatcher::sendNextRequest()
{
	if (m_currentRequest || m_pendingRequests.isEmpty() || m_failed)
	{
		return;
	}

	const auto pendingRequest = m_pendingRequests.dequeue();

	// the finished() signal is emitted from the event loop even if the call is finished already
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

	if (watcher->isError())
	{
		// the request has been dropped, later ones are still valid
		if (m_errorReported == false)
		{
			vWarning() << "portal input request failed:" << watcher->error().name() << watcher->error().message();
			m_errorReported = true;
		}
	}
	else
	{
		m_errorReported = false;
	}

	sendNextRequest();
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
