// SPDX-FileCopyrightText: 2024-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PipeWireVncConfiguration.h"
#include "PluginInterface.h"
#include "VncServerPluginInterface.h"

#include <atomic>

/**
 * @brief Wayland VNC server plugin using XDG Desktop Portal RemoteDesktop + PipeWire.
 *
 * Screen capture and input run in the separate process veyon-capture-helper
 * (PipeWireCaptureServer, 02-ipc.md §1a.5) since xdg-desktop-portal rejects the non-dumpable
 * veyon-server. In veyon-server this plugin only starts and supervises the helper, which serves
 * RFB on a private Unix domain socket (no TCP port) with a per-start random VNC password;
 * veyon-server only talks to it after checking that the listener is the helper process.
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

	// the helper listens on a Unix domain socket only (localSocketPath()), never on a TCP port
	int configuredServerPort() override
	{
		return -1;
	}

	// random per server start, so VncServer uses it for all connections to the helper
	Password configuredPassword() override;

	QString localSocketPath() override;

	qint64 localSocketProcessId() override
	{
		return m_helperProcessId;
	}

private:
	// password and socket name are generated once per server start, first from the main thread
	// before the VncServer thread starts (VncServer::password()/endpoint())
	void initializeHelperCredentials();

	PipeWireVncConfiguration m_configuration;

	Password m_vncPassword;
	QString m_socketName;
	std::atomic<qint64> m_helperProcessId{-1};
};
