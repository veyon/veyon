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

#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QObject>
#include <QString>
#include <QVariantMap>

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
 *   flatpak permission-set kde-authorized remote-desktop io.veyon.Veyon.Server yes
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

	void connectResponseSignal(const QDBusObjectPath& requestPath);
	void disconnectResponseSignal();

	void setState(State s);

	static QString makeRequestToken();
	static QString senderToken();

	QDBusInterface* m_portalInterface{nullptr};
	QString m_sessionHandle;
	QString m_currentRequestToken;
	QString m_restoreToken;
	State m_state{State::Idle};
	int m_pipewireFd{-1};
	quint32 m_pipeWireNodeId{0};
	QString m_connectedRequestPath;  // path of the currently connected Response signal
};
