// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QThread>

#include "CryptoCore.h"
#include "VncServerSocket.h"

class VncServerPluginInterface;

class VncServer : public QThread
{
	Q_OBJECT
public:
	using Password = CryptoCore::SecureArray;

	explicit VncServer(QObject* parent = nullptr);
	~VncServer() override;

	void prepare();

	int serverBasePort() const;
	int serverPort() const;

	// loopback TCP port or, for veyon-capture-helper, its Unix domain socket
	VncServerEndpoint endpoint() const;

	Password password() const;

private:
	void run() override;

	void queryExternalVncServerPassword();

	VncServerPluginInterface* m_pluginInterface;

	// provided by veyon-service when the external VNC server plugin is configured (02-ipc.md §6)
	Password m_externalVncServerPassword;
};
