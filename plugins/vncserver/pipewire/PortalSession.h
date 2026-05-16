/*
 * PortalSession.h - XDG Desktop Portal RemoteDesktop session management
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

#pragma once

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "OrgFreedesktopPortalRemoteDesktop.h"
#include "OrgFreedesktopPortalRequest.h"
#include "OrgFreedesktopPortalScreenCast.h"
#include "OrgFreedesktopPortalSession.h"

/**
 * @brief Manages an XDG Desktop Portal RemoteDesktop session.
 *
 * Follows the portal flow:
 *   CreateSession → SelectDevices → SelectSources → Start → OpenPipeWireRemote
 *
 * All portal responses arrive asynchronously via the /org/freedesktop/portal/desktop/response
 * signal and are dispatched to the appropriate handler.
 *
 * The app_id used for permission-store lookups is "io.veyon.Veyon.Server".
 * KDE Plasma admins can pre-authorize unattended access via:
 *   flatpak permission-set kde-authorized screencast io.veyon.Veyon.Server yes
 * or directly through the DBus PermissionStore interface.
 */
class PortalSession : public QObject
{
	Q_OBJECT

public:
	enum class State {
		Idle,
		CreatingSession,
		SelectingDevices,
		SelectingSources,
		Starting,
		Running,
		Failed
	};
	Q_ENUM(State)

	explicit PortalSession(QObject* parent = nullptr);
	~PortalSession() override;

	/**
	 * @brief Start the portal session flow.
	 * Emits started() on success or failed() on error.
	 */
	void start();

	/**
	 * @brief Close and clean up the portal session.
	 */
	void close();

	State state() const { return m_state; }

	/**
	 * @brief PipeWire remote file descriptor (valid after started() signal).
	 * The caller takes ownership (must close when done).
	 */
	int pipewireFd() const { return m_pipewireFd; }

	/**
	 * @brief PipeWire node ID for the screen stream (valid after started() signal).
	 */
	quint32 pipeWireNodeId() const { return m_pipeWireNodeId; }

	/**
	 * @brief Forward a keyboard keysym event from a VNC client to the portal session.
	 *
	 * The @p keySym value is the X11 keysym received from the VNC client (rfbKeySym).
	 * No-op when the session is not in the Running state.
	 */
	void notifyKeyboardKeysym(quint32 keySym, bool down);

	/**
	 * @brief Forward a pointer (mouse) event from a VNC client to the portal session.
	 *
	 * Sends NotifyPointerMotionAbsolute when the position changes and
	 * NotifyPointerButton / NotifyPointerAxisDiscrete when button state changes.
	 * No-op when the session is not in the Running state.
	 *
	 * @p buttonMask is the LibVNCServer button bitmask (bit 0 = left, 1 = middle,
	 * 2 = right, 3 = scroll-up, 4 = scroll-down, 5 = scroll-left, 6 = scroll-right,
	 * 7 = side, 8 = extra).
	 */
	void notifyPointer(int buttonMask, int x, int y);

Q_SIGNALS:
	void started();
	void failed();

private Q_SLOTS:
	void onPortalResponse(uint response, const QVariantMap& results);

private:
	void createSession();
	void selectDevices();
	void selectSources();
	void startSession();
	void openPipeWireRemote();

	void connectResponseSignal(const QString& requestPath);
	void disconnectResponseSignal();

	void setState(State s);

	QString makeRequestToken() const;
	QString senderToken() const;
	QString makeRequestPath(const QString& token) const;

	QDBusConnection m_sessionBus;
	OrgFreedesktopPortalRemoteDesktopInterface* m_remoteDesktop{nullptr};
	OrgFreedesktopPortalScreenCastInterface* m_screenCast{nullptr};
	OrgFreedesktopPortalRequestInterface* m_requestInterface{nullptr};

	QString m_sessionHandle;
	QString m_restoreToken;
	State m_state{State::Idle};
	int m_pipewireFd{-1};
	quint32 m_pipeWireNodeId{0};
	QString m_connectedRequestPath;

	int m_lastButtonMask{0};
	double m_lastPointerX{0.0};
	double m_lastPointerY{0.0};

};
