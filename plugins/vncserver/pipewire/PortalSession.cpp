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

#include <linux/input.h>

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QRandomGenerator>

#include "PortalSession.h"
#include "VeyonCore.h"

// XDG Desktop Portal service and interface names
static const auto PortalService = QStringLiteral("org.freedesktop.portal.Desktop");
static const auto PortalObjectPath = QStringLiteral("/org/freedesktop/portal/desktop");
static const auto ConnectionName = QStringLiteral("PipeWirePortalSession");

PortalSession::PortalSession(QObject* parent)
	: QObject(parent)
	, m_sessionBus(QDBusConnection::connectToBus(QDBusConnection::SessionBus, ConnectionName))
{
	const auto appId = QGuiApplication::desktopFileName();

	QProcess::execute(QStringLiteral("flatpak"),
					  {QStringLiteral("permission-set"), QStringLiteral("kde-authorized"),
					   QStringLiteral("screencast"), appId, QStringLiteral("yes")});
	QProcess::execute(QStringLiteral("flatpak"),
					  {QStringLiteral("permission-set"), QStringLiteral("kde-authorized"),
					   QStringLiteral("remote-desktop"), appId, QStringLiteral("yes")});

	if (!m_sessionBus.registerService(appId))
	{
		vWarning() << "Could not register D-Bus service name" << appId
				   << ":" << m_sessionBus.lastError().message();
	}

	m_remoteDesktop = new OrgFreedesktopPortalRemoteDesktopInterface(PortalService, PortalObjectPath, m_sessionBus, this);
	m_screenCast = new OrgFreedesktopPortalScreenCastInterface(PortalService, PortalObjectPath, m_sessionBus, this);
}



PortalSession::~PortalSession()
{
	close();
	m_sessionBus.unregisterService(QGuiApplication::desktopFileName());
	m_sessionBus.disconnectFromBus(ConnectionName);
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



void PortalSession::createSession()
{
	setState(State::CreatingSession);

	const QString token = makeRequestToken();

	QVariantMap options;
	options[QStringLiteral("handle_token")] = token;
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
	connect(new QDBusPendingCallWatcher(call, this), &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
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

QString PortalSession::makeRequestToken() const
{
	return QStringLiteral("veyon_%1").arg(QRandomGenerator::global()->generate());
}

QString PortalSession::senderToken() const
{
	return m_sessionBus.baseService().remove(QLatin1Char(':')).replace(QLatin1Char('.'), QLatin1Char('_'));
}

QString PortalSession::makeRequestPath(const QString& token) const
{
	return QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(senderToken(), token);
}

// ---------------------------------------------------------------------------
// Input event forwarding
// ---------------------------------------------------------------------------

void PortalSession::notifyKeyboardKeysym(quint32 keySym, bool down)
{
	if (m_state != State::Running)
	{
		return;
	}

	// Portal state: 0 = released, 1 = pressed
	m_remoteDesktop->NotifyKeyboardKeysym(
		QDBusObjectPath(m_sessionHandle),
		QVariantMap{},
		static_cast<int>(keySym),
		down ? 1u : 0u);
}

void PortalSession::notifyPointer(int buttonMask, int x, int y)
{
	if (m_state != State::Running || m_pipeWireNodeId == 0)
	{
		return;
	}

	const auto sessionPath = QDBusObjectPath(m_sessionHandle);
	const double dx = static_cast<double>(x);
	const double dy = static_cast<double>(y);

	// Send motion event when the position changed
	if (dx != m_lastPointerX || dy != m_lastPointerY)
	{
		m_remoteDesktop->NotifyPointerMotionAbsolute(
			sessionPath, QVariantMap{}, m_pipeWireNodeId, dx, dy);
		m_lastPointerX = dx;
		m_lastPointerY = dy;
	}

	// Send button/scroll events for every bit that changed
	if (buttonMask != m_lastButtonMask)
	{
		// Linux evdev button codes matching VNC button mask bits 0–8
		// Bits 3-6 are scroll directions (no evdev button code — use axis)
		static const int evdevButtons[] = {
			BTN_LEFT,   // bit 0
			BTN_MIDDLE, // bit 1
			BTN_RIGHT,  // bit 2
			0,          // bit 3 – scroll up   (axis event)
			0,          // bit 4 – scroll down (axis event)
			0,          // bit 5 – scroll left (axis event)
			0,          // bit 6 – scroll right (axis event)
			BTN_SIDE,   // bit 7
			BTN_EXTRA,  // bit 8
		};
		static constexpr int numButtons = static_cast<int>(sizeof(evdevButtons) / sizeof(evdevButtons[0]));

		for (int i = 0; i < numButtons; ++i)
		{
			const int prev    = (m_lastButtonMask >> i) & 1;
			const int current = (buttonMask >> i) & 1;
			if (prev == current)
			{
				continue;
			}

			if (evdevButtons[i] != 0)
			{
				// Regular button press/release (0=released, 1=pressed)
				m_remoteDesktop->NotifyPointerButton(
					sessionPath, QVariantMap{}, evdevButtons[i], static_cast<uint>(current));
			}
			else if (current != 0)
			{
				// Scroll axis: only fire on press (the release has no discrete step)
				int axis  = 0; // 0 = vertical
				int steps = 0;
				switch (i)
				{
				case 3: axis = 0; steps = -1; break; // scroll up
				case 4: axis = 0; steps =  1; break; // scroll down
				case 5: axis = 1; steps = -1; break; // scroll left
				case 6: axis = 1; steps =  1; break; // scroll right
				default: break;
				}
				if (steps != 0)
				{
					m_remoteDesktop->NotifyPointerAxisDiscrete(
						sessionPath, QVariantMap{}, static_cast<uint>(axis), steps);
				}
			}
		}

		m_lastButtonMask = buttonMask;
	}
}

