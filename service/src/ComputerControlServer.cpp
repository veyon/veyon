/*
 * ComputerControlServer.cpp - implementation of ComputerControlServer
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include "ItalcCore.h"

#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#endif

#include <QCoreApplication>
#include <QHostInfo>

#include "AccessControlProvider.h"
#include "ComputerControlServer.h"
#include "ComputerControlClient.h"
#include "FeatureMessage.h"
#include "ItalcConfiguration.h"
#include "LocalSystem.h"
#include "SystemTrayIcon.h"
#include "VariantStream.h"


ComputerControlServer::ComputerControlServer() :
	QObject(),
	m_allowedIPs(),
	m_failedAuthHosts(),
	m_pluginManager(),
	m_builtinFeatures( m_pluginManager ),
	m_featureManager( m_pluginManager ),
	m_featureWorkerManager( m_featureManager ),
	m_serverAuthenticationManager( this ),
	m_serverAccessControlManager( m_featureWorkerManager, m_builtinFeatures.desktopAccessDialog(), this ),
	m_vncServer( m_pluginManager, ItalcCore::config().vncServerPort() ),
	m_vncProxyServer( m_vncServer.serverPort(),
					  m_vncServer.password(),
					  ItalcCore::config().localConnectOnly() || AccessControlProvider().isAccessDeniedByLocalState() ?
						  QHostAddress::LocalHost : QHostAddress::Any,
					  ItalcCore::config().computerControlServerPort(),
					  this,
					  this )
{
	m_builtinFeatures.systemTrayIcon().setToolTip(
				tr( "%1 Service %2 at %3:%4" ).
				arg( ItalcCore::applicationName() ).
				arg( ITALC_VERSION ).
				arg( QHostInfo::localHostName() ).
				arg( QString::number( ItalcCore::config().computerControlServerPort() ) ),
				m_featureWorkerManager );

	// make app terminate once the VNC server thread has finished
	connect( &m_vncServer, &VncServer::finished, QCoreApplication::instance(), &QCoreApplication::quit );

	connect( &m_serverAuthenticationManager, &ServerAuthenticationManager::authenticationError,
			 this, &ComputerControlServer::showAuthenticationErrorMessage );
}



ComputerControlServer::~ComputerControlServer()
{
}



void ComputerControlServer::start()
{
	m_vncServer.start();
	m_vncProxyServer.start();
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
		qDebug( "ComputerControlServer::handleFeatureMessage(): could not read feature message!" );
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

	if( ItalcCore::config().disabledFeatures().contains( featureMessage.featureUid().toString() ) )
	{
		qWarning() << "ComputerControlServer::handleFeatureMessage(): ignoring message as feature"
				   << featureMessage.featureUid() << "is disabled by configuration!";
		return false;
	}

	qDebug() << "ComputerControlServer::handleFeatureMessage():"
			 << featureMessage.featureUid()
			 << featureMessage.command()
			 << featureMessage.arguments();

	return m_featureManager.handleServiceFeatureMessage( featureMessage, m_featureWorkerManager );
}



void ComputerControlServer::showAuthenticationErrorMessage( QString host, QString user )
{
	qWarning() << "ComputerControlServer: failed authenticating client" << host << user;

	QMutexLocker l( &m_dataMutex );

	if( m_failedAuthHosts.contains( host ) == false )
	{
		m_failedAuthHosts += host;
		m_builtinFeatures.systemTrayIcon().showMessage(
					tr( "Authentication error" ),
					tr( "User %1 (IP: %2) tried to access this computer "
						"but could not authenticate successfully!" ).arg( user ).arg( host ),
					m_featureWorkerManager );
	}
}
