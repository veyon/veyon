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

#include "ComputerControlServer.h"
#include "ComputerControlClient.h"
#include "FeatureMessage.h"
#include "LocalSystem.h"
#include "SystemTrayIcon.h"
#include "VariantStream.h"

//ItalcCore::config->localConnectOnly() || AccessControlProvider().isAccessDeniedByLocalState()

ComputerControlServer::ComputerControlServer() :
	QObject(),
	m_allowedIPs(),
	m_failedAuthHosts(),
	m_pluginManager(),
	m_builtinFeatures( m_pluginManager ),
	m_featureManager( m_pluginManager ),
	m_featureWorkerManager( m_featureManager ),
	m_serverAuthenticationManager( m_featureWorkerManager, m_builtinFeatures.desktopAccessDialog() ),
	m_vncServer( 12345 ),
	m_vncProxyServer( m_vncServer.serverPort(), m_vncServer.password(), ItalcCore::serverPort, this, this )
{
	m_builtinFeatures.systemTrayIcon().setToolTip(
				tr( "%1 Service %2 at %3:%4" ).
				arg( ItalcCore::applicationName() ).
				arg( ITALC_VERSION ).
				arg( QHostInfo::localHostName() ).
				arg( QString::number( ItalcCore::serverPort ) ),
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



bool ComputerControlServer::handleCoreMessage( QTcpSocket* socket )
{
	char messageType;
	socket->read( &messageType, sizeof(messageType) );

			// receive message
	ItalcCore::Msg msgIn( socket );
	msgIn.receive();

	qDebug() << "ComputerControlServer: received message" << msgIn.cmd()
			 << "with arguments" << msgIn.args();

	const QString cmd = msgIn.cmd();
	if( cmd == ItalcCore::GetUserInformation )
	{
		static QString lastUserName, lastFullUsername;

		LocalSystem::User user = LocalSystem::User::loggedOnUser();
		QString currentUsername = user.name();
		if( lastUserName != currentUsername )
		{
			lastUserName = currentUsername;
			lastFullUsername = user.fullName();
		}
		if( !lastFullUsername.isEmpty() &&
				currentUsername != lastFullUsername )
		{
			currentUsername = QString( "%1 (%2)" ).arg( currentUsername ).
					arg( lastFullUsername );
		}
		ItalcCore::Msg( socket, ItalcCore::UserInformation ).
				addArg( "username", currentUsername ).
				addArg( "homedir", user.homePath() ).
				send();
	}
	else if( cmd == ItalcCore::ExecCmds )
	{
		const QString cmds = msgIn.arg( "cmds" );
		if( !cmds.isEmpty() )
		{
			LocalSystem::User user = LocalSystem::User::loggedOnUser();
			LocalSystem::Process proc(
						LocalSystem::Process::findProcessId( QString(), -1, &user ) );

			foreach( const QString & cmd, cmds.split( '\n' ) )
			{
				LocalSystem::Process::Handle hProcess =
						proc.runAsUser( cmd,
										LocalSystem::Desktop::activeDesktop().name() );
#ifdef ITALC_BUILD_WIN32
				if( hProcess )
				{
					CloseHandle( hProcess );
				}
#else
				(void) hProcess;
#endif
			}
		}
	}
	else if( cmd == ItalcCore::LogonUserCmd )
	{
		LocalSystem::logonUser( msgIn.arg( "uname" ),
								msgIn.arg( "passwd" ),
								msgIn.arg( "domain" ) );
	}
	else if( cmd == ItalcCore::LogoutUser )
	{
		LocalSystem::logoutUser();
	}
	else if( cmd == ItalcCore::SetRole )
	{
		const int role = msgIn.arg( "role" ).toInt();
		if( role > ItalcCore::RoleNone && role < ItalcCore::RoleCount )
		{
			ItalcCore::role = static_cast<ItalcCore::UserRoles>( role );
		}
	}
	else
	{
		qCritical() << "ComputerControlServer::handleItalcClientMessage(...): could not handle cmd" << cmd;
	}

	return true;
}



bool ComputerControlServer::handleFeatureMessage( QTcpSocket* socket )
{
	char messageType;
	socket->getChar( &messageType );

	// receive message
	FeatureMessage featureMessage( socket );
	if( featureMessage.isReadyForReceive() == false )
	{
		socket->ungetChar( messageType );
		return false;
	}

	featureMessage.receive();

	qDebug() << "ComputerControlServer::handleItalcFeatureMessage():"
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
