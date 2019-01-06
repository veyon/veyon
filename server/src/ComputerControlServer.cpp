/*
 * ComputerControlServer.cpp - implementation of ComputerControlServer
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

#include <QCoreApplication>
#include <QHostInfo>

#include "AccessControlProvider.h"
#include "ComputerControlServer.h"
#include "ComputerControlClient.h"
#include "FeatureMessage.h"
#include "VeyonConfiguration.h"
#include "SystemTrayIcon.h"


ComputerControlServer::ComputerControlServer( QObject* parent ) :
	QObject( parent ),
	m_allowedIPs(),
	m_failedAuthHosts(),
	m_builtinFeatures(),
	m_featureManager(),
	m_featureWorkerManager( *this, m_featureManager ),
	m_serverAuthenticationManager( this ),
	m_serverAccessControlManager( m_featureWorkerManager, m_builtinFeatures.desktopAccessDialog(), this ),
	m_vncServer(),
	m_vncProxyServer( VeyonCore::config().localConnectOnly() || AccessControlProvider().isAccessToLocalComputerDenied() ?
						  QHostAddress::LocalHost : QHostAddress::Any,
					  VeyonCore::config().primaryServicePort() + VeyonCore::sessionId(),
					  this,
					  this )
{
	m_builtinFeatures.systemTrayIcon().setToolTip(
				tr( "%1 Service %2 at %3:%4" ).arg( VeyonCore::applicationName(), VeyonCore::version(),
					QHostInfo::localHostName(),
					QString::number( VeyonCore::config().primaryServicePort() + VeyonCore::sessionId() ) ),
				m_featureWorkerManager );

	// make app terminate once the VNC server thread has finished
	connect( &m_vncServer, &VncServer::finished, QCoreApplication::instance(), &QCoreApplication::quit );

	connect( &m_serverAuthenticationManager, &ServerAuthenticationManager::authenticationDone,
			 this, &ComputerControlServer::showAuthenticationMessage );
}



ComputerControlServer::~ComputerControlServer()
{
	qDebug(Q_FUNC_INFO);

	m_vncProxyServer.stop();
}



bool ComputerControlServer::start()
{
	if( m_vncProxyServer.start( m_vncServer.serverPort(), m_vncServer.password() ) == false )
	{
		return false;
	}

	m_vncServer.prepare();
	m_vncServer.start();

	return true;
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

	return m_featureManager.handleFeatureMessage( *this, featureMessage );
}



bool ComputerControlServer::sendFeatureMessageReply( const FeatureMessage& request, const FeatureMessage& reply )
{
	qDebug() << Q_FUNC_INFO << reply.featureUid() << reply.command() << reply.arguments();

	char rfbMessageType = rfbVeyonFeatureMessage;
	request.ioDevice()->write( &rfbMessageType, sizeof(rfbMessageType) );

	return reply.send( request.ioDevice() );
}



void ComputerControlServer::showAuthenticationMessage( ServerAuthenticationManager::AuthResult result, const QString& host, const QString& user )
{
	if( result == ServerAuthenticationManager::AuthResultSuccessful )
	{
		qInfo() << "ComputerControlServer: successfully authenticated" << user << "at host" << host;

		if( VeyonCore::config().remoteConnectionNotificationsEnabled() )
		{
			m_builtinFeatures.systemTrayIcon().showMessage(
						tr( "Remote access" ),
						tr( "User \"%1\" at host \"%2\" is now accessing this computer." ).arg( user, host ),
						m_featureWorkerManager );
		}
	}
	else if( result == ServerAuthenticationManager::AuthResultFailed )
	{
		qWarning() << "ComputerControlServer: failed authenticating client" << host << user;

		if( VeyonCore::config().failedAuthenticationNotificationsEnabled() )
		{
			QMutexLocker l( &m_dataMutex );

			if( m_failedAuthHosts.contains( host ) == false )
			{
				m_failedAuthHosts += host;
				m_builtinFeatures.systemTrayIcon().showMessage(
							tr( "Authentication error" ),
							tr( "User \"%1\" at host \"%2\" attempted to access this computer "
								"but could not authenticate successfully!" ).arg( user, host ),
							m_featureWorkerManager );
			}
		}
	}
	else
	{
		qCritical() << Q_FUNC_INFO << "Invalid auth result" << result;
	}
}
