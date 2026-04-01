/*
 * PortalSession.cpp - XDG Desktop Portal RemoteDesktop session management
 *
 * Copyright (c) 2024-2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "PortalSession.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QRandomGenerator>

#include <VeyonCore.h>

// XDG Desktop Portal service and interface names
static const QString PortalService      = QStringLiteral("org.freedesktop.portal.Desktop");
static const QString PortalObjectPath   = QStringLiteral("/org/freedesktop/portal/desktop");
static const QString RemoteDesktopIface = QStringLiteral("org.freedesktop.portal.RemoteDesktop");
static const QString ScreenCastIface    = QStringLiteral("org.freedesktop.portal.ScreenCast");
static const QString RequestIface       = QStringLiteral("org.freedesktop.portal.Request");

PortalSession::PortalSession(QObject* parent)
	: QObject(parent)
{
	m_portalInterface = new QDBusInterface(
		PortalService,
		PortalObjectPath,
		RemoteDesktopIface,
		QDBusConnection::sessionBus(),
		this
	);
}

PortalSession::~PortalSession()
{
	close();
}

void PortalSession::start()
{
	if (m_state != State::Idle && m_state != State::Failed)
	{
		return;
	}
	m_pipewireFd = -1;
	m_pipeWireNodeId = 0;
	m_sessionHandle.clear();
	createSession();
}

void PortalSession::close()
{
	disconnectResponseSignal();

	if (!m_sessionHandle.isEmpty())
	{
		QDBusInterface sessionIface(
			PortalService,
			m_sessionHandle,
			QStringLiteral("org.freedesktop.portal.Session"),
			QDBusConnection::sessionBus()
		);
		sessionIface.call(QStringLiteral("Close"));
		m_sessionHandle.clear();
	}

	setState(State::Idle);
}

// ---------------------------------------------------------------------------
// Portal flow steps
// ---------------------------------------------------------------------------

void PortalSession::createSession()
{
	setState(State::CreatingSession);

	const QString token = makeRequestToken();
	const QString requestPath = makeRequestPath(token);

	// Connect to Response BEFORE making the call to avoid the race where the
	// portal emits the signal before we register the listener.
	connectResponseSignal(requestPath);

	QVariantMap options;
	options[QStringLiteral("handle_token")] = token;
	options[QStringLiteral("session_handle_token")] = makeRequestToken();

	auto call = m_portalInterface->asyncCall(QStringLiteral("CreateSession"), options);
	auto* watcher = new QDBusPendingCallWatcher(call, this);
	connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
		w->deleteLater();
		QDBusPendingReply<QDBusObjectPath> reply = *w;
		if (reply.isError())
		{
			vCritical() << "CreateSession call failed:" << reply.error().message();
			disconnectResponseSignal();
			setState(State::Failed);
			Q_EMIT failed();
		}
	});
}

void PortalSession::selectDevices()
{
	setState(State::SelectingDevices);

	const QString token = makeRequestToken();
	const QString requestPath = makeRequestPath(token);

	connectResponseSignal(requestPath);

	QVariantMap options;
	options[QStringLiteral("handle_token")] = token;
	// Request keyboard + pointer devices (1=keyboard, 2=pointer, 3=both)
	options[QStringLiteral("types")] = QVariant::fromValue<uint>(3u);
	// persist_mode 2 = persist until explicitly revoked
	options[QStringLiteral("persist_mode")] = QVariant::fromValue<uint>(2u);
	if (!m_restoreToken.isEmpty())
	{
		options[QStringLiteral("restore_token")] = m_restoreToken;
	}

	auto call = m_portalInterface->asyncCall(
		QStringLiteral("SelectDevices"),
		QVariant::fromValue(QDBusObjectPath(m_sessionHandle)),
		options
	);
	auto* watcher = new QDBusPendingCallWatcher(call, this);
	connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
		w->deleteLater();
		QDBusPendingReply<QDBusObjectPath> reply = *w;
		if (reply.isError())
		{
			vCritical() << "SelectDevices call failed:" << reply.error().message();
			disconnectResponseSignal();
			setState(State::Failed);
			Q_EMIT failed();
		}
	});
}

void PortalSession::selectSources()
{
	setState(State::SelectingSources);

	const QString token = makeRequestToken();
	const QString requestPath = makeRequestPath(token);

	connectResponseSignal(requestPath);

	// SelectSources is on the ScreenCast interface; the RemoteDesktop session
	// handle is shared with it.
	QDBusInterface screenCastIface(
		PortalService,
		PortalObjectPath,
		ScreenCastIface,
		QDBusConnection::sessionBus()
	);

	QVariantMap options;
	options[QStringLiteral("handle_token")] = token;
	options[QStringLiteral("types")]        = QVariant::fromValue<uint>(1u); // 1=monitor
	options[QStringLiteral("multiple")]     = false;
	options[QStringLiteral("persist_mode")] = QVariant::fromValue<uint>(2u);
	if (!m_restoreToken.isEmpty())
	{
		options[QStringLiteral("restore_token")] = m_restoreToken;
	}

	auto call = screenCastIface.asyncCall(
		QStringLiteral("SelectSources"),
		QVariant::fromValue(QDBusObjectPath(m_sessionHandle)),
		options
	);
	auto* watcher = new QDBusPendingCallWatcher(call, this);
	connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
		w->deleteLater();
		QDBusPendingReply<QDBusObjectPath> reply = *w;
		if (reply.isError())
		{
			vCritical() << "SelectSources call failed:" << reply.error().message();
			disconnectResponseSignal();
			setState(State::Failed);
			Q_EMIT failed();
		}
	});
}

void PortalSession::startSession()
{
	setState(State::Starting);

	const QString token = makeRequestToken();
	const QString requestPath = makeRequestPath(token);

	connectResponseSignal(requestPath);

	QVariantMap options;
	options[QStringLiteral("handle_token")] = token;

	auto call = m_portalInterface->asyncCall(
		QStringLiteral("Start"),
		QVariant::fromValue(QDBusObjectPath(m_sessionHandle)),
		QStringLiteral(""), // parent_window handle (empty for daemon/unattended use)
		options
	);
	auto* watcher = new QDBusPendingCallWatcher(call, this);
	connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
		w->deleteLater();
		QDBusPendingReply<QDBusObjectPath> reply = *w;
		if (reply.isError())
		{
			vCritical() << "Start call failed:" << reply.error().message();
			disconnectResponseSignal();
			setState(State::Failed);
			Q_EMIT failed();
		}
	});
}

void PortalSession::openPipeWireRemote()
{
	QDBusInterface screenCastIface(
		PortalService,
		PortalObjectPath,
		ScreenCastIface,
		QDBusConnection::sessionBus()
	);

	QDBusReply<QDBusUnixFileDescriptor> reply = screenCastIface.call(
		QStringLiteral("OpenPipeWireRemote"),
		QVariant::fromValue(QDBusObjectPath(m_sessionHandle)),
		QVariantMap{}
	);

	if (!reply.isValid())
	{
		vCritical() << "OpenPipeWireRemote failed:" << reply.error().message();
		setState(State::Failed);
		Q_EMIT failed();
		return;
	}

	// dup() the FD so we own it independently of the QDBus reply lifetime
	m_pipewireFd = ::dup(reply.value().fileDescriptor());
	if (m_pipewireFd < 0)
	{
		vCritical() << "dup() of PipeWire FD failed";
		setState(State::Failed);
		Q_EMIT failed();
		return;
	}

	setState(State::Running);
	Q_EMIT started();
}

// ---------------------------------------------------------------------------
// Response signal handling
// ---------------------------------------------------------------------------

void PortalSession::connectResponseSignal(const QString& requestPath)
{
	disconnectResponseSignal();

	m_connectedRequestPath = requestPath;

	const bool ok = QDBusConnection::sessionBus().connect(
		PortalService,
		m_connectedRequestPath,
		RequestIface,
		QStringLiteral("Response"),
		this,
		SLOT(onPortalResponse(uint, QVariantMap))
	);

	if (!ok)
	{
		vWarning() << "Failed to connect to Response signal on" << m_connectedRequestPath;
		m_connectedRequestPath.clear();
	}
}

void PortalSession::disconnectResponseSignal()
{
	if (m_connectedRequestPath.isEmpty())
	{
		return;
	}

	QDBusConnection::sessionBus().disconnect(
		PortalService,
		m_connectedRequestPath,
		RequestIface,
		QStringLiteral("Response"),
		this,
		SLOT(onPortalResponse(uint, QVariantMap))
	);

	m_connectedRequestPath.clear();
}

void PortalSession::onPortalResponse(uint response, const QVariantMap& results)
{
	disconnectResponseSignal();

	if (response != 0)
	{
		vWarning() << "Portal request in state" << static_cast<int>(m_state)
		           << "denied/cancelled (response=" << response << ")";
		setState(State::Failed);
		Q_EMIT failed();
		return;
	}

	// Save restore token whenever the portal provides one
	if (results.contains(QStringLiteral("restore_token")))
	{
		m_restoreToken = results.value(QStringLiteral("restore_token")).toString();
		vDebug() << "Got portal restore_token:" << m_restoreToken;
	}

	switch (m_state)
	{
	case State::CreatingSession:
	{
		// session_handle can arrive as QDBusObjectPath or as a plain string
		QDBusObjectPath sessionPath = results.value(QStringLiteral("session_handle")).value<QDBusObjectPath>();
		m_sessionHandle = sessionPath.path().isEmpty()
		                  ? results.value(QStringLiteral("session_handle")).toString()
		                  : sessionPath.path();

		if (m_sessionHandle.isEmpty())
		{
			vCritical() << "CreateSession response missing session_handle";
			setState(State::Failed);
			Q_EMIT failed();
			return;
		}
		vDebug() << "Portal session created:" << m_sessionHandle;
		selectDevices();
		break;
	}

	case State::SelectingDevices:
		selectSources();
		break;

	case State::SelectingSources:
		startSession();
		break;

	case State::Starting:
	{
		// The streams result is an array of (node_id, properties) structs.
		// Extract the first node ID.
		const QDBusArgument streams = results.value(QStringLiteral("streams")).value<QDBusArgument>();
		if (!streams.currentSignature().isEmpty())
		{
			streams.beginArray();
			if (!streams.atEnd())
			{
				streams.beginStructure();
				streams >> m_pipeWireNodeId;
				streams.endStructure();
			}
			streams.endArray();
		}

		vDebug() << "Portal session started, PipeWire node ID:" << m_pipeWireNodeId;
		openPipeWireRemote();
		break;
	}

	default:
		vWarning() << "Unexpected portal response in state" << static_cast<int>(m_state);
		break;
	}
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void PortalSession::setState(State s)
{
	m_state = s;
}

QString PortalSession::makeRequestToken()
{
	return QStringLiteral("veyon_%1").arg(QRandomGenerator::global()->generate());
}

QString PortalSession::senderToken()
{
	// The portal request path uses the D-Bus unique bus name with ':' removed
	// and '.' replaced by '_'.  E.g. ":1.23" -> "1_23".
	QString sender = QDBusConnection::sessionBus().baseService();
	sender.remove(QLatin1Char(':'));
	sender.replace(QLatin1Char('.'), QLatin1Char('_'));
	return sender;
}

QString PortalSession::makeRequestPath(const QString& token)
{
	return QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2")
	       .arg(senderToken(), token);
}

