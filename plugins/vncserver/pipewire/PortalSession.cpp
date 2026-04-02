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

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QRandomGenerator>

#include <VeyonCore.h>

// XDG Desktop Portal service and interface names
static const auto PortalService    = QStringLiteral("org.freedesktop.portal.Desktop");
static const auto PortalObjectPath = QStringLiteral("/org/freedesktop/portal/desktop");

// Stable application identifier used for:
//   - well-known D-Bus service name registration (so xdg-desktop-portal can
//     identify this process when doing kde-authorized PermissionStore lookups)
//   - KDE pre-authorization: flatpak permission-set kde-authorized screencast
//                            io.veyon.Veyon.Server yes
static const auto AppId          = QStringLiteral("io.veyon.Veyon.Server");
static const auto ConnectionName = QStringLiteral("PipeWirePortalSession");

PortalSession::PortalSession(QObject* parent)
	: QObject(parent)
	, m_sessionBus(QDBusConnection::connectToBus(QDBusConnection::SessionBus, ConnectionName))
{
	// Register the stable well-known bus name.  xdg-desktop-portal identifies
	// non-Flatpak callers by their well-known D-Bus service name; without this
	// registration the portal sees an empty app_id and the kde-authorized
	// PermissionStore entry stored under "io.veyon.Veyon.Server" is never matched.
	if (!m_sessionBus.registerService(AppId))
	{
		vWarning() << "Could not register D-Bus service name" << AppId
		           << ":" << m_sessionBus.lastError().message();
	}

	m_remoteDesktop = new OrgFreedesktopPortalRemoteDesktopInterface(
		PortalService, PortalObjectPath, m_sessionBus, this);
	m_screenCast = new OrgFreedesktopPortalScreenCastInterface(
		PortalService, PortalObjectPath, m_sessionBus, this);
}

PortalSession::~PortalSession()
{
	close();
	m_sessionBus.unregisterService(AppId);
	QDBusConnection::disconnectFromBus(ConnectionName);
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
		OrgFreedesktopPortalSessionInterface sessionIface(
			PortalService, m_sessionHandle, m_sessionBus);
		sessionIface.Close();
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

	QVariantMap options;
	options[QStringLiteral("handle_token")]         = token;
	options[QStringLiteral("session_handle_token")] = makeRequestToken();

	// Subscribe to the Response signal BEFORE making the async call to avoid a
	// race condition where the portal emits Response before we can connect.
	connectResponseSignal(makeRequestPath(token));

	auto call = m_remoteDesktop->CreateSession(options);
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

	connectResponseSignal(makeRequestPath(token));

	auto call = m_remoteDesktop->SelectDevices(QDBusObjectPath(m_sessionHandle), options);
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

	QVariantMap options;
	options[QStringLiteral("handle_token")] = token;
	options[QStringLiteral("types")]        = QVariant::fromValue<uint>(1u); // 1=monitor
	options[QStringLiteral("multiple")]     = false;
	// ScreenCast interface only supports persist_mode=0; persist_mode=2 is
	// only valid on the RemoteDesktop interface (used in selectDevices()).
	options[QStringLiteral("persist_mode")] = QVariant::fromValue<uint>(0u);

	connectResponseSignal(makeRequestPath(token));

	auto call = m_screenCast->SelectSources(QDBusObjectPath(m_sessionHandle), options);
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

	QVariantMap options;
	options[QStringLiteral("handle_token")] = token;

	connectResponseSignal(makeRequestPath(token));

	auto call = m_remoteDesktop->Start(
		QDBusObjectPath(m_sessionHandle),
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
	auto pending = m_screenCast->OpenPipeWireRemote(
		QDBusObjectPath(m_sessionHandle), QVariantMap{});
	pending.waitForFinished();

	if (pending.isError())
	{
		vCritical() << "OpenPipeWireRemote failed:" << pending.error().message();
		setState(State::Failed);
		Q_EMIT failed();
		return;
	}

	// dup() the FD so we own it independently of the QDBus reply lifetime
	m_pipewireFd = ::dup(pending.value().fileDescriptor());
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
	m_requestInterface = new OrgFreedesktopPortalRequestInterface(
		PortalService, requestPath, m_sessionBus, this);

	connect(m_requestInterface, &OrgFreedesktopPortalRequestInterface::Response,
	        this, &PortalSession::onPortalResponse);
}

void PortalSession::disconnectResponseSignal()
{
	delete m_requestInterface;
	m_requestInterface = nullptr;
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

QString PortalSession::senderToken() const
{
	return m_sessionBus.baseService().remove(QLatin1Char(':')).replace(QLatin1Char('.'), QLatin1Char('_'));
}

QString PortalSession::makeRequestPath(const QString& token) const
{
	// The portal Request object path is:
	//   /org/freedesktop/portal/desktop/request/<sender>/<token>
	// where <sender> is the caller's unique D-Bus name with dots and colons
	// replaced by underscores (e.g. ':1.123' → '_1_123').
	return QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(senderToken(), token);
}
