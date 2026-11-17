// SPDX-FileCopyrightText: 2024-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMutex>
#include <QObject>

#include "crypto/SecureBuffer.h"

class PortalSession;
class PipeWireFramebuffer;
class QSocketNotifier;

extern "C" {
#include <rfb/rfb.h>
}

/**
 * @brief Wayland screen capture and input via XDG Desktop Portal RemoteDesktop + PipeWire.
 *
 * Acquires a screen capture via the org.freedesktop.portal.RemoteDesktop D-Bus API
 * and feeds the resulting PipeWire stream into a LibVNCServer rfbScreen which is reachable via a
 * Unix domain socket only, so that veyon-server can connect and receive screen updates. LibVNCServer
 * itself listens on no TCP/UDP port (and thus serves no WebSockets); connections are accepted here
 * and handed to it only if the peer runs with the helper's uid.
 *
 * Runs in veyon-capture-helper only (02-ipc.md §1a.5): xdg-desktop-portal rejects the
 * non-dumpable veyon-server, so all portal calls and the PipeWire stream live in this separate,
 * dumpable process which holds no Veyon secret except its per-start VNC password.
 *
 * The helper uses the stable application identifier "io.veyon.veyon-server".
 * KDE Plasma 6.3+ administrators can pre-authorize unattended access with:
 *
 *   flatpak permission-set kde-authorized screencast io.veyon.veyon-server yes
 *
 * See plugins/vncserver/pipewire/README.md for full pre-authorization details.
 */
class PipeWireCaptureServer : public QObject
{
	Q_OBJECT
public:
	explicit PipeWireCaptureServer(bool persistRestoreToken, QObject* parent = nullptr);
	~PipeWireCaptureServer() override;

	// concurrent RFB clients (veyon-server connections and the demo server)
	static constexpr int MaximumClients = 16;
	// maximum time LibVNCServer blocks in a read/write of a single client
	static constexpr int MaximumClientWait = 2000;

	// blocks; returns false if the RFB server can't be set up, e.g. because the socket exists
	bool run(const QString& socketPath, const VeyonCrypto::SecureBuffer& vncPassword);

private Q_SLOTS:
	void onPortalStarted();
	void onPortalFailed();
	void onStreamEnded();

private:
	bool initVncServer(const VeyonCrypto::SecureBuffer& vncPassword);
	bool listenOnSocket(const QString& socketPath);
	void acceptConnections();
	void cleanupVncServer();

	static rfbNewClientAction onNewClient(rfbClientPtr client);
	static void onClientGone(rfbClientPtr client);

	static void rfbLogDebug(const char* format, ...);
	static void rfbLogNone(const char* format, ...);

	// VNC authentication against the binary per-start password; libvncserver's
	// rfbCheckPasswordByList() would truncate it at the first NUL byte
	static rfbBool checkVncPassword(rfbClientPtr client, const char* response, int length);

	// LibVNCServer input event callbacks (C linkage function pointers)
	static void onKbdAddEvent(rfbBool down, rfbKeySym keySym, rfbClientRec* cl);
	static void onPtrAddEvent(int buttonMask, int x, int y, rfbClientRec* cl);

	const bool m_persistRestoreToken;

	PortalSession* m_portalSession{nullptr};
	PipeWireFramebuffer* m_framebuffer{nullptr};
	rfbScreenInfoPtr m_rfbScreen{nullptr};
	VeyonCrypto::SecureBuffer m_vncPassword;
	QString m_socketPath;
	int m_listenSocket{-1};
	QSocketNotifier* m_listenNotifier{nullptr};
	int m_clientCount{0};

	QMutex m_screenMutex;

	bool m_serverRunning{false};
	bool m_shouldRestart{false};
};
