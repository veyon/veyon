/*
 * PipeWireVncServer.h - Wayland VNC server backend using PipeWire + XDG Desktop Portal
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

#include "PluginInterface.h"
#include "VncServerPluginInterface.h"

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
 * The plugin uses the stable application identifier "io.veyon.Veyon.Server".
 * KDE Plasma 6.3+ administrators can pre-authorize unattended access with:
 *
 *   flatpak permission-set kde-authorized remote-desktop io.veyon.Veyon.Server yes
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
		return Plugin::Uid{ QStringLiteral("3b8e5c1a-9f72-4d3e-b6a0-2c7f1e8d4b95") };
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
		return tr("Wayland VNC server (PipeWire/XDG Portal)");
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
		return Plugin::NoFlags;
	}

	QStringList supportedSessionTypes() const override
	{
		return { QStringLiteral("wayland") };
	}

	QWidget* configurationWidget() override
	{
		return nullptr;
	}

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

private:
	bool initVncServer(int serverPort, const Password& password);
	void cleanupVncServer();

	static void rfbLogDebug(const char* format, ...);
	static void rfbLogNone(const char* format, ...);

	PortalSession*       m_portalSession{nullptr};
	PipeWireFramebuffer* m_framebuffer{nullptr};
	rfbScreenInfoPtr     m_rfbScreen{nullptr};
	char*                m_framebufferData{nullptr};
	char*                m_vncPassword{nullptr};
	char*                m_vncPasswords[2]{nullptr, nullptr};

	bool m_serverRunning{false};
	bool m_shouldRestart{false};
};
