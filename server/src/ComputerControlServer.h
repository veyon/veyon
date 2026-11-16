// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMap>
#include <QMutex>
#include <QSet>
#include <QtConcurrent>

#include "FeatureWorkerManager.h"
#include "RfbVeyonAuth.h"
#include "ServerAccessControlManager.h"
#include "ServerAuthenticationManager.h"
#include "VeyonServerInterface.h"
#include "VncProxyConnectionFactory.h"
#include "VncProxyServer.h"
#include "VncServer.h"

class ComputerControlClient;

class ComputerControlServer : public VeyonServerInterface, VncProxyConnectionFactory
{
	Q_OBJECT
public:
	explicit ComputerControlServer(QObject* parent = nullptr);
	~ComputerControlServer() override;

	bool start();

	VncProxyConnection* createVncProxyConnection(QTcpSocket* clientSocket,
												 int vncServerPort,
												 const Password& vncServerPassword,
												 QObject* parent) override;

	ServerAuthenticationManager& authenticationManager()
	{
		return m_serverAuthenticationManager;
	}

	ServerAccessControlManager& accessControlManager()
	{
		return m_serverAccessControlManager;
	}

	bool handleFeatureMessage(ComputerControlClient* client);

	bool sendFeatureMessageReply(const MessageContext& context, const FeatureMessage& reply) override;

	FeatureWorkerManager& featureWorkerManager() override
	{
		return m_featureWorkerManager;
	}

	int vncServerBasePort() const override
	{
		return m_vncServer.serverBasePort();
	}

	void setMinimumFramebufferUpdateInterval(const MessageContext& context, int interval) override;

private:
	void checkForIncompleteAuthentication(VncServerClient* client);
	void showAuthenticationMessage(VncServerClient* client);
	void showAccessControlMessage(VncServerClient* client);
	QFutureWatcher<QMap<QString, QString>>* resolveFQDNs(const QStringList& hosts);

	void sendAsyncFeatureMessages(VncProxyConnection* connection);
	void updateTrayIconToolTip();

	QMutex m_dataMutex;
	QStringList m_allowedIPs;

	QMap<QString, QString> m_resolvedHostNames;
	QSet<QString> m_pendingHostResolutions;
	static constexpr auto MaximumResolvedHostNames = 1024;

	QStringList m_failedAuthHosts;
	QStringList m_failedAccessControlHosts;

	FeatureWorkerManager m_featureWorkerManager;

	ServerAuthenticationManager m_serverAuthenticationManager;
	ServerAccessControlManager m_serverAccessControlManager;

	VncServer m_vncServer;
	VncProxyServer m_vncProxyServer;
};
