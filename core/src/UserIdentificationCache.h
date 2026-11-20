// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QList>
#include <QString>
#include <QUuid>

#include "MessageContext.h"

// server-side state of the IdentifyUser feature: keeps the identity entered by the user of the
// session in memory for the lifetime of the server process and collects the requests of all
// masters while the identification dialog is open, so that only one dialog is shown at a time
class UserIdentificationCache
{
public:
	enum class RequestResult {
		Cached,		 // identity is known, reply immediately
		StartDialog, // send a new request (requestId()) to the session worker
		Pending		 // a dialog is already open, reply once it has been answered
	};

	// sessionUser is the login name of the session's user - a cached identity is only returned
	// as long as the session user did not change
	RequestResult addRequest(const MessageContext& context, const QString& sessionUser, bool dialogRunning)
	{
		if (m_identity.isEmpty() == false && m_identitySessionUser == sessionUser)
		{
			return RequestResult::Cached;
		}

		m_identity.clear();
		m_identitySessionUser.clear();

		bool alreadyPending = false;
		for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end();)
		{
			if (it->ioDevice() == nullptr)
			{
				// connection has been closed in the meantime
				it = m_pendingRequests.erase(it);
				continue;
			}
			alreadyPending |= it->ioDevice() == context.ioDevice();
			++it;
		}

		// each connection is answered once only, no matter how often it asks
		if (alreadyPending == false && context.ioDevice())
		{
			m_pendingRequests.append(context);
		}

		// (re)start the dialog if none has been requested yet or the worker went away without replying
		if (m_requestId.isNull() || dialogRunning == false)
		{
			m_requestId = QUuid::createUuid();
			return RequestResult::StartDialog;
		}

		return RequestResult::Pending;
	}

	// to be called with the identity entered in the dialog - returns all requests to be answered;
	// an empty identity (e.g. dialog cancelled) is not cached so that the user is asked again
	QList<MessageContext> complete(const QString& identity, const QString& sessionUser)
	{
		if (m_requestId.isNull())
		{
			// no request pending
			return {};
		}

		if (identity.isEmpty() == false)
		{
			m_identity = identity;
			m_identitySessionUser = sessionUser;
		}

		m_requestId = QUuid{};

		QList<MessageContext> requests;
		requests.swap(m_pendingRequests);
		return requests;
	}

	bool isRequestPending() const
	{
		return m_requestId.isNull() == false;
	}

	QUuid requestId() const
	{
		return m_requestId;
	}

	const QString& identity() const
	{
		return m_identity;
	}

private:
	QString m_identity;
	QString m_identitySessionUser;
	QUuid m_requestId;
	QList<MessageContext> m_pendingRequests;
};
