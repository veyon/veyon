/*
 * ComputerControlServer.cpp - implementation of ComputerControlServer
 *
 * Copyright (c) 2006-2017 Tobias Junghans <tobydox@users.sf.net>
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

#include <QCoreApplication>
#include <QHostInfo>

#include "AccessControlProvider.h"
#include "ComputerControlServer.h"
#include "ComputerControlClient.h"
#include "FeatureMessage.h"
#include "VeyonConfiguration.h"
#include "LocalSystem.h"
#include "SystemTrayIcon.h"


ComputerControlServer::ComputerControlServer( QObject* parent ) :
	QObject( parent ),
	m_allowedIPs(),
	m_failedAuthHosts(),
	m_builtinFeatures(),
	m_featureManager(),
	m_featureWorkerManager( m_featureManager ),
	m_serverAuthenticationManager( this ),
	m_serverAccessControlManager( m_featureWorkerManager, m_builtinFeatures.desktopAccessDialog(), this ),
	m_vncServer(),
	m_vncProxyServer( VeyonCore::config().localConnectOnly() || AccessControlProvider().isAccessToLocalComputerDenied() ?
						  QHostAddress::LocalHost : QHostAddress::Any,
					  VeyonCore::config().primaryServicePort(),
					  this,
					  this )
{
	m_builtinFeatures.systemTrayIcon().setToolTip(
				tr( "%1 Service %2 at %3:%4" ).arg( VeyonCore::applicationName(), QStringLiteral(VEYON_VERSION),
					QHostInfo::localHostName(),
					QString::number( VeyonCore::config().primaryServicePort() ) ),
				m_featureWorkerManager );

	// make app terminate once the VNC server thread has finished
	connect( &m_vncServer, &VncServer::finished, QCoreApplication::instance(), &QCoreApplication::quit );

	connect( &m_serverAuthenticationManager, &ServerAuthenticationManager::authenticationError,
			 this, &ComputerControlServer::showAuthenticationErrorMessage );
}



ComputerControlServer::~ComputerControlServer()
{
	qDebug(Q_FUNC_INFO);

	m_vncProxyServer.stop();
}



void ComputerControlServer::start()
{
	m_vncServer.start();
	m_vncProxyServer.start( m_vncServer.serverPort(), m_vncServer.password() );
}



VncProxyConnection* ComputerControlServer::createVncProxyConnection( QTcpSocket* clientSocket,
																	 int vncServerPort,
																	 const QString& vncServerPassword,
																	 QObject* parent )
{
	return new ComputerControlClient( this, clientSocket, vncServerPort, vncServerPassword, parent );
}



bool ComputerControlServer::handleFeatureMessage( QTcpSocket* socket )
{
	char messageType;
	if( socket->getChar( &messageType ) == false )
	{
		qWarning( "ComputerControlServer::handleFeatureMessage(): could not read feature message!" );
		return false;
	}

	// receive message
	FeatureMessage featureMessage( socket );
	if( featureMessage.isReadyForReceive() == false )
	{
		socket->ungetChar( messageType );
		return false;
	}

	featureMessage.receive();

	return m_featureManager.handleServiceFeatureMessage( featureMessage, m_featureWorkerManager );
}



void ComputerControlServer::showAuthenticationErrorMessage( const QString& host, const QString& user )
{
	qWarning() << "ComputerControlServer: failed authenticating client" << host << user;

	QMutexLocker l( &m_dataMutex );

	if( m_failedAuthHosts.contains( host ) == false )
	{
		m_failedAuthHosts += host;
		m_builtinFeatures.systemTrayIcon().showMessage(
					tr( "Authentication error" ),
					tr( "User %1 (IP: %2) tried to access this computer "
						"but could not authenticate successfully!" ).arg( user, host ),
					m_featureWorkerManager );
	}
}
