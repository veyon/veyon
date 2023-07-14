/*
 * ComputerControlServer.cpp - implementation of ComputerControlServer
 *
 * Copyright (c) 2006-2023 Tobias Junghans <tobydox@veyon.io>
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

#include <QCoreApplication>

#include "AccessControlProvider.h"
#include "BuiltinFeatures.h"
#include "ComputerControlClient.h"
#include "ComputerControlServer.h"
#include "FeatureManager.h"
#include "FeatureMessage.h"
#include "HostAddress.h"
#include "VeyonConfiguration.h"
#include "SystemTrayIcon.h"


ComputerControlServer::ComputerControlServer( QObject* parent ) :
	VeyonServerInterface(parent),
	m_featureWorkerManager( *this ),
	m_serverAuthenticationManager( this ),
	m_serverAccessControlManager( m_featureWorkerManager, VeyonCore::builtinFeatures().desktopAccessDialog(), this ),
	m_vncProxyServer( VeyonCore::config().localConnectOnly() || AccessControlProvider().isAccessToLocalComputerDenied() ?
						  QHostAddress::LocalHost : QHostAddress::Any,
					  VeyonCore::config().veyonServerPort() + VeyonCore::sessionId(),
					  this,
					  this )
{
	updateTrayIconToolTip();

	// make app terminate once the VNC server thread has finished
	connect( &m_vncServer, &VncServer::finished, QCoreApplication::instance(), &QCoreApplication::quit );

	connect( &m_serverAuthenticationManager, &ServerAuthenticationManager::finished,
			 this, &ComputerControlServer::showAuthenticationMessage );

	connect( &m_serverAccessControlManager, &ServerAccessControlManager::finished,
			 this, &ComputerControlServer::showAccessControlMessage );

	connect(&m_vncProxyServer, &VncProxyServer::serverMessageProcessed,
			 this, &ComputerControlServer::sendAsyncFeatureMessages, Qt::DirectConnection);
	connect( &m_vncProxyServer, &VncProxyServer::connectionClosed, this, &ComputerControlServer::updateTrayIconToolTip );
}



ComputerControlServer::~ComputerControlServer()
{
	vDebug();

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
																	 const Password& vncServerPassword,
																	 QObject* parent )
{
	auto client = new ComputerControlClient( this, clientSocket, vncServerPort, vncServerPassword, parent );

	connect( client, &ComputerControlClient::serverConnectionClosed, this,
		[=]() { checkForIncompleteAuthentication( client->serverClient() ); },
		Qt::DirectConnection );

	return client;
}



bool ComputerControlServer::handleFeatureMessage(ComputerControlClient* client)
{
	auto socket = client->proxyClientSocket();

	char messageType;
	if( socket->getChar( &messageType ) == false )
	{
		vWarning() << "could not read feature message!";
		return false;
	}

	// receive message
	FeatureMessage featureMessage;
	if( featureMessage.isReadyForReceive( socket ) == false )
	{
		socket->ungetChar( messageType );
		return false;
	}

	if (featureMessage.receive(socket) == false)
	{
		return false;
	}

	VeyonCore::featureManager().handleFeatureMessage( *this, MessageContext{socket, client}, featureMessage );

	return true;
}



bool ComputerControlServer::sendFeatureMessageReply( const MessageContext& context, const FeatureMessage& reply )
{
	vDebug() << reply;

	if( context.ioDevice() )
	{
		char rfbMessageType = FeatureMessage::RfbMessageType;
		context.ioDevice()->write( &rfbMessageType, sizeof(rfbMessageType) );

		return reply.send( context.ioDevice() );
	}

	return false;
}



void ComputerControlServer::setMinimumFramebufferUpdateInterval(const MessageContext& context, int interval)
{
	auto client = qobject_cast<ComputerControlClient *>(context.connection());
	if (client)
	{
		client->setMinimumFramebufferUpdateInterval(interval);
	}
}



void ComputerControlServer::checkForIncompleteAuthentication( VncServerClient* client )
{
	// connection to client closed during authentication?
	if( client->protocolState() == VncServerProtocol::State::AuthenticationMethods ||
		client->protocolState() == VncServerProtocol::State::Authenticating )
	{
		// then mark as failed authentication and report it
		client->setAuthState( VncServerClient::AuthState::Failed );

		showAuthenticationMessage( client );
	}
}



void ComputerControlServer::showAuthenticationMessage( VncServerClient* client )
{
	if( client->authState() == VncServerClient::AuthState::Failed )
	{
		vWarning() << "Authentication failed for" << client->hostAddress() << client->username();

		if (VeyonCore::config().failedAuthenticationNotificationsEnabled() &&
			VeyonCore::platform().sessionFunctions().currentSessionHasUser())
		{
			QMutexLocker l( &m_dataMutex );

			if( m_failedAuthHosts.contains( client->hostAddress() ) == false )
			{
				m_failedAuthHosts += client->hostAddress();

				const auto fqdn = HostAddress( client->hostAddress() ).tryConvert( HostAddress::Type::FullyQualifiedDomainName );

				VeyonCore::builtinFeatures().systemTrayIcon().showMessage(
							tr( "Authentication error" ),
							tr( "User \"%1\" at host \"%2\" attempted to access this computer "
								"but could not authenticate successfully." ).arg( client->username(), fqdn ),
							m_featureWorkerManager );
			}
		}
	}
}



void ComputerControlServer::showAccessControlMessage( VncServerClient* client )
{
	if( client->accessControlState() == VncServerClient::AccessControlState::Successful &&
		client->protocolState() == VncServerProtocol::State::AccessControl )
	{
		vInfo() << "Access control successful for" << client->hostAddress() << client->username();

		if (VeyonCore::config().remoteConnectionNotificationsEnabled() &&
			VeyonCore::platform().sessionFunctions().currentSessionHasUser())
		{
			const auto fqdn = HostAddress( client->hostAddress() ).tryConvert( HostAddress::Type::FullyQualifiedDomainName );

			VeyonCore::builtinFeatures().systemTrayIcon().showMessage(
						tr( "Remote access" ),
						tr( "User \"%1\" at host \"%2\" is now accessing this computer." ).
						arg( client->username(), fqdn ),
						m_featureWorkerManager );
		}

		updateTrayIconToolTip();
	}
	else if( client->accessControlState() == VncServerClient::AccessControlState::Failed )
	{
		vWarning() << "Access control failed for" << client->hostAddress() << client->username();

		if( VeyonCore::config().failedAuthenticationNotificationsEnabled() )
		{
			QMutexLocker l( &m_dataMutex );

			if( m_failedAccessControlHosts.contains( client->hostAddress() ) == false )
			{
				m_failedAccessControlHosts += client->hostAddress();

				const auto fqdn = HostAddress( client->hostAddress() ).tryConvert( HostAddress::Type::FullyQualifiedDomainName );

				VeyonCore::builtinFeatures().systemTrayIcon().showMessage(
							tr( "Access control error" ),
							tr( "User \"%1\" at host \"%2\" attempted to access this computer "
								"but has been blocked due to access control settings." ).
							arg( client->username(), fqdn ),
							m_featureWorkerManager );
			}
		}
	}
}



QFutureWatcher<void>* ComputerControlServer::resolveFQDNs( const QStringList& hosts )
{
	auto watcher = new QFutureWatcher<void>();

	watcher->setFuture( QtConcurrent::run( [this, hosts]() {
		for( const auto& host : hosts )
		{
			const auto fqdn = HostAddress( host ).tryConvert( HostAddress::Type::FullyQualifiedDomainName );

			m_dataMutex.lock();
			m_resolvedHostNames[host] = fqdn;
			m_dataMutex.unlock();
		}
	} ) );

	connect( watcher, &QFutureWatcher<void>::finished, watcher, &QObject::deleteLater );

	return watcher;
}



void ComputerControlServer::sendAsyncFeatureMessages(VncProxyConnection* connection)
{
	VeyonCore::featureManager().sendAsyncFeatureMessages(*this, MessageContext{connection->proxyClientSocket()});
}



void ComputerControlServer::updateTrayIconToolTip()
{
	if (VeyonCore::platform().sessionFunctions().currentSessionHasUser() == false)
	{
		return;
	}

	auto toolTip = tr( "%1 Service %2 at %3:%4" ).arg( VeyonCore::applicationName(), VeyonCore::versionString(),
													   HostAddress::localFQDN(),
												QString::number( VeyonCore::config().veyonServerPort() + VeyonCore::sessionId() ) );

	QMutexLocker locker( &m_dataMutex );

	QStringList clients;
	QStringList hostsToResolve;
	clients.reserve( m_vncProxyServer.clients().size() );
	hostsToResolve.reserve( m_vncProxyServer.clients().size() );

	for( const auto* client : m_vncProxyServer.clients() )
	{
		const auto clientIpAddress = client->proxyClientSocket()->peerAddress().toString();
		if( m_resolvedHostNames.contains( clientIpAddress ) == false )
		{
			hostsToResolve.append( clientIpAddress );
			clients.append( clientIpAddress );
		}
		else
		{
			clients.append( m_resolvedHostNames[clientIpAddress] );
		}
	}

	if( hostsToResolve.isEmpty() == false )
	{
		auto watcher = resolveFQDNs( hostsToResolve );
		connect( watcher, &QFutureWatcher<void>::finished, this, &ComputerControlServer::updateTrayIconToolTip );
	}

	if( clients.isEmpty() == false )
	{
		toolTip += QLatin1Char('\n') + tr( "Active connections:") + QLatin1Char('\n') + clients.join( QLatin1Char('\n') );
	}

	VeyonCore::builtinFeatures().systemTrayIcon().setToolTip( toolTip, m_featureWorkerManager );
}
