// SPDX-FileCopyrightText: 2024-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMutex>

#include "PipeWireVncConfiguration.h"
#include "PluginInterface.h"
#include "VncServerPluginInterface.h"

#include <atomic>

class PortalSession;
class PipeWireFramebuffer;

extern "C" {
#include <rfb/rfb.h>
}

/**
 * @brief Wayland VNC server plugin using XDG Desktop Portal RemoteDesktop + PipeWire.
 *
 * Acquires a screen capture via the org.freedesktop.portal.RemoteDesktop D-Bus API
 * and feeds the resulting PipeWire stream into a LibVNCServer rfbScreen so that VNC
 * clients can connect and receive screen updates.
 *
 * The plugin uses the stable application identifier "io.veyon.veyon-server".
 * KDE Plasma 6.3+ administrators can pre-authorize unattended access with:
 *
 *   flatpak permission-set kde-authorized screencast io.veyon.veyon-server yes
 *
 * See plugins/vncserver/pipewire/README.md for full pre-authorization details.
 */
class PipeWireVncServer : public QObject, VncServerPluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.PipeWireVncServer")
	Q_INTERFACES(PluginInterface VncServerPluginInterface)

public:
	explicit PipeWireVncServer(QObject* parent = nullptr);
	~PipeWireVncServer() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("3b8e5c1a-9f72-4d3e-b6a0-2c7f1e8d4b95")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 0);
	}

	QString name() const override
	{
		return QStringLiteral("PipeWireVncServer");
	}

	QString description() const override
	{
		return tr("Wayland VNC server (PipeWire/XDG Desktop Portal)");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	Plugin::Flags flags() const override
	{
		return Plugin::ProvidesDefaultImplementation;
	}

	QStringList supportedSessionTypes() const override
	{
		return {QStringLiteral("wayland")};
	}

	QWidget* configurationWidget() override;

	void prepareServer() override;

	bool runServer(int serverPort, const Password& password) override;

	int configuredServerPort() override
	{
		return -1;
	}

	Password configuredPassword() override
	{
		return {};
	}

private Q_SLOTS:
	void onPortalStarted();
	void onPortalFailed();
	void onStreamEnded();
	void onFrameSizeChanged(QSize size);

private:
	void closeAndRestartSession();
	bool initVncServer(int serverPort, const Password& password);
	void cleanupVncServer();

	static void rfbLogDebug(const char* format, ...);
	static void rfbLogNone(const char* format, ...);

	// LibVNCServer input event callbacks (C linkage function pointers)
	static void onKbdAddEvent(rfbBool down, rfbKeySym keySym, rfbClientRec* cl);
	static void onPtrAddEvent(int buttonMask, int x, int y, rfbClientRec* cl);

	PipeWireVncConfiguration m_configuration;

	PortalSession* m_portalSession{nullptr};
	PipeWireFramebuffer* m_framebuffer{nullptr};
	rfbScreenInfoPtr m_rfbScreen{nullptr};
	char* m_vncPassword{nullptr};
	char* m_vncPasswords[2]{nullptr, nullptr};

	QMutex m_screenMutex;

	bool m_serverRunning{false};
	bool m_shouldRestart{false};
	std::atomic<int> m_connectedClients{0};
};
