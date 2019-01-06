/*
 * ComputerControlServer.h - header file for ComputerControlServer
 *
 * Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef COMPUTER_CONTROL_SERVER_H
#define COMPUTER_CONTROL_SERVER_H

#include <QtCore/QMutex>
#include <QtCore/QStringList>

#include "BuiltinFeatures.h"
#include "FeatureManager.h"
#include "FeatureWorkerManager.h"
#include "RfbVeyonAuth.h"
#include "ServerAuthenticationManager.h"
#include "ServerAccessControlManager.h"
#include "VeyonServerInterface.h"
#include "VncProxyServer.h"
#include "VncProxyConnectionFactory.h"
#include "VncServer.h"

class ComputerControlServer : public QObject, VncProxyConnectionFactory, VeyonServerInterface
{
	Q_OBJECT
public:
	ComputerControlServer( QObject* parent = nullptr );
	~ComputerControlServer() override;

	bool start();

	VncProxyConnection* createVncProxyConnection( QTcpSocket* clientSocket,
												  int vncServerPort,
												  const QString& vncServerPassword,
												  QObject* parent ) override;

	ServerAuthenticationManager& authenticationManager()
	{
		return m_serverAuthenticationManager;
	}

	ServerAccessControlManager& accessControlManager()
	{
		return m_serverAccessControlManager;
	}

	bool handleFeatureMessage( QTcpSocket* socket );

	bool sendFeatureMessageReply( const FeatureMessage& request, const FeatureMessage& reply ) override;

	void setAllowedIPs( const QStringList &allowedIPs );

	FeatureWorkerManager& featureWorkerManager() override
	{
		return m_featureWorkerManager;
	}


private:
	void showAuthenticationMessage( ServerAuthenticationManager::AuthResult result, const QString& host, const QString& user );

	QMutex m_dataMutex;
	QStringList m_allowedIPs;

	QStringList m_failedAuthHosts;

	BuiltinFeatures m_builtinFeatures;
	FeatureManager m_featureManager;
	FeatureWorkerManager m_featureWorkerManager;

	ServerAuthenticationManager m_serverAuthenticationManager;
	ServerAccessControlManager m_serverAccessControlManager;

	VncServer m_vncServer;
	VncProxyServer m_vncProxyServer;

} ;

#endif
