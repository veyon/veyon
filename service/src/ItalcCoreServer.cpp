/*
 * ItalcCoreServer.cpp - ItalcCoreServer
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


#include <italcconfig.h>

#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#include <psapi.h>
#endif

#include <QtCore/QCoreApplication>
#include <QtNetwork/QHostInfo>

#include "ItalcCoreServer.h"
#include "AccessControlProvider.h"
#include "DesktopAccessPermission.h"
#include "DsaKey.h"
#include "FeatureMessage.h"
#include "ItalcRfbExt.h"
#include "LocalSystem.h"
#include "SystemTrayIcon.h"


ItalcCoreServer * ItalcCoreServer::_this = NULL;


ItalcCoreServer::ItalcCoreServer() :
	QObject(),
	m_allowedIPs(),
	m_failedAuthHosts(),
	m_pluginManager(),
	m_builtinFeatures( m_pluginManager ),
	m_featureManager( m_pluginManager ),
	m_featureWorkerManager( m_featureManager ),
	m_slaveManager()
{
	Q_ASSERT( _this == NULL );
	_this = this;

	m_builtinFeatures.systemTrayIcon().setToolTip(
				tr( "%1 Service %2 at %3:%4" ).
				arg( ItalcCore::applicationName() ).
				arg( ITALC_VERSION ).
				arg( QHostInfo::localHostName() ).
				arg( QString::number( ItalcCore::serverPort ) ),
				m_featureWorkerManager );
}




ItalcCoreServer::~ItalcCoreServer()
{
	_this = NULL;
}




bool ItalcCoreServer::handleItalcCoreMessage( SocketDispatcher sock,
												void *user )
{
	SocketDevice sdev( sock, user );

	// receive message
	ItalcCore::Msg msgIn( &sdev );
	msgIn.receive();

	qDebug() << "ItalcCoreServer: received message" << msgIn.cmd()
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
		ItalcCore::Msg( &sdev, ItalcCore::UserInformation ).
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
	else if( cmd == ItalcCore::DisableLocalInputs )
	{
		// TODO
		//LocalSystem::disableLocalInputs( msgIn.arg( "disabled" ).toInt() );
	}
	else if( cmd == ItalcCore::SetRole )
	{
		const int role = msgIn.arg( "role" ).toInt();
		if( role > ItalcCore::RoleNone && role < ItalcCore::RoleCount )
		{
			ItalcCore::role = static_cast<ItalcCore::UserRoles>( role );
		}
	}
	else if( cmd == ItalcCore::StartDemo )
	{
		QString host = msgIn.arg( "host" );
		QString port = msgIn.arg( "port" );
		// no host given?
		if( host.isEmpty() )
		{
			// then guess IP from remote peer address
			const int MAX_HOST_LEN = 255;
			char hostArr[MAX_HOST_LEN+1];
			sock( hostArr, MAX_HOST_LEN, SocketGetPeerAddress, user );
			hostArr[MAX_HOST_LEN] = 0;
			host = hostArr;
		}
		if( port.isEmpty() )
		{
			port = QString::number( PortOffsetDemoServer );
		}
		if( !host.contains( ':' ) )
		{
			host += ':' + port;
		}
		m_slaveManager.startDemo( host, msgIn.arg( "fullscreen" ).toInt() );
	}
	else if( cmd == ItalcCore::StopDemo )
	{
		m_slaveManager.stopDemo();
	}
	else if( cmd == ItalcCore::LockScreen )
	{
		m_slaveManager.lockScreen();
	}
	else if( cmd == ItalcCore::UnlockScreen )
	{
		m_slaveManager.unlockScreen();
	}
	else if( cmd == ItalcCore::LockInput )
	{
		m_slaveManager.lockInput();
	}
	else if( cmd == ItalcCore::UnlockInput )
	{
		m_slaveManager.unlockInput();
	}
	else if( cmd == ItalcCore::StartDemoServer )
	{
		ItalcCore::authenticationCredentials->setCommonSecret(
									DsaKey::generateChallenge().toBase64() );
		m_slaveManager.demoServerMaster()->start(
			msgIn.arg( "sourceport" ).toInt(),
			msgIn.arg( "destinationport" ).toInt() );
	}
	else if( cmd == ItalcCore::StopDemoServer )
	{
		m_slaveManager.demoServerMaster()->stop();
	}
	else if( cmd == ItalcCore::DemoServerAllowHost )
	{
		m_slaveManager.demoServerMaster()->allowHost( msgIn.arg( "host" ) );
	}
	else if( cmd == ItalcCore::DemoServerUnallowHost )
	{
		m_slaveManager.demoServerMaster()->unallowHost( msgIn.arg( "host" ) );
	}
	else if( cmd == ItalcCore::ReportSlaveStateFlags )
	{
		ItalcCore::Msg( &sdev, cmd ).
				addArg( "slavestateflags", m_slaveManager.slaveStateFlags() ).
					send();
	}
	else
	{
		qCritical() << "ItalcCoreServer::handleItalcClientMessage(...): could not handle cmd" << cmd;
	}

	return true;
}



bool ItalcCoreServer::handleItalcFeatureMessage( SocketDispatcher socketDispatcher, void *user )
{
	SocketDevice socketDevice( socketDispatcher, user );

	// receive message
	FeatureMessage featureMessage( &socketDevice );
	featureMessage.receive();

	qDebug() << "ItalcCoreServer::handleItalcFeatureMessage():" << featureMessage.featureUid()
			 << "with arguments" << featureMessage.arguments();

	return m_featureManager.handleServiceFeatureMessage( featureMessage, &socketDevice, m_featureWorkerManager );
}



bool ItalcCoreServer::authSecTypeItalc( SocketDispatcher sd, void *user )
{
	// find out IP of host - needed at several places
	const int MAX_HOST_LEN = 255;
	char host[MAX_HOST_LEN+1];
	sd( host, MAX_HOST_LEN, SocketGetPeerAddress, user );
	host[MAX_HOST_LEN] = 0;

	SocketDevice sdev( sd, user );

	// send list of supported authentication types - can't use QList<QVariant>
	// here due to a strange bug in Qt
	QMap<QString, QVariant> supportedAuthTypes;
	supportedAuthTypes["ItalcAuthDSA"] = ItalcAuthDSA;
	supportedAuthTypes["ItalcAuthHostBased"] = ItalcAuthHostBased;
	if( ItalcCore::authenticationCredentials->hasCredentials(
									AuthenticationCredentials::CommonSecret ) )
	{
		supportedAuthTypes["ItalcAuthCommonSecret"] = ItalcAuthCommonSecret;
	}
	sdev.write( supportedAuthTypes );

	uint32_t result = rfbVncAuthFailed;
	ItalcAuthTypes chosen = static_cast<ItalcAuthTypes>( sdev.read().toInt() );
	if( !supportedAuthTypes.values().contains( chosen ) )
	{
		errorMsgAuth( host );
		qCritical( "ItalcCoreServer::authSecTypeItalc(...): "
				"client chose unsupported authentication type %d!", (int) chosen );
		return false;
	}

	const QString username = sdev.read().toString();

	switch( chosen )
	{
		// no authentication
		case ItalcAuthNone:
			result = rfbVncAuthOK;
			break;

		// host has to be in list of allowed hosts
		case ItalcAuthHostBased:
			if( doHostBasedAuth( host ) )
			{
				result = rfbVncAuthOK;
			}
			break;

		// authentication via DSA-challenge/-response
		case ItalcAuthDSA:
			if( doKeyBasedAuth( sdev, host ) &&
					performAccessControl( username, host, DesktopAccessPermission::KeyAuthentication ) )
			{
				result = rfbVncAuthOK;
			}
			break;

		case ItalcAuthCommonSecret:
			if( doCommonSecretAuth( sdev ) )
			{
				result = rfbVncAuthOK;
			}
			break;

		default:
			break;
	}

	if( result != rfbVncAuthOK )
	{
		// only report about failed authentications for regular authentication methods
		if( chosen != ItalcAuthHostBased )
		{
			errorMsgAuth( host );
		}
		qWarning() << "ItalcCoreServer::authSecTypeItalc(): failed authenticating client" << host;
		return false;
	}

	return true;
}



void ItalcCoreServer::setAllowedIPs(const QStringList &allowedIPs)
{
	QMutexLocker l( &m_dataMutex );
	m_allowedIPs = allowedIPs;
}



bool ItalcCoreServer::performAccessControl( const QString &username, const QString &host,
											DesktopAccessPermission::AuthenticationMethod authenticationMethod )
{
	auto accessResult = AccessControlProvider().checkAccess( username, host );

	DesktopAccessPermission desktopAccessPermission( authenticationMethod );

	switch( accessResult )
	{
	case AccessControlProvider::AccessToBeConfirmed:
		return desktopAccessPermission.ask( username, host );

	case AccessControlProvider::AccessAllow:
		if( desktopAccessPermission.authenticationMethodRequiresConfirmation() )
		{
			return desktopAccessPermission.ask( username, host );
		}
		return true;

	default:
		break;
	}

	return false;
}



void ItalcCoreServer::errorMsgAuth( const QString &ip )
{
	QMutexLocker l( &m_dataMutex );

	if( m_failedAuthHosts.contains( ip ) == false )
	{
		m_failedAuthHosts += ip;
		m_builtinFeatures.systemTrayIcon().showMessage(
					tr( "Authentication error" ),
					tr( "Somebody (IP: %1) tried to access this computer "
						"but could not authenticate successfully!" ).arg( ip ),
					m_featureWorkerManager );
	}
}




bool ItalcCoreServer::doKeyBasedAuth( SocketDevice &sdev, const QString &host )
{
	// generate data to sign and send to client
	const QByteArray chall = DsaKey::generateChallenge();
	sdev.write( QVariant( chall ) );

	// get user-role
	const ItalcCore::UserRoles urole =
				static_cast<ItalcCore::UserRoles>( sdev.read().toInt() );

	// now try to verify received signed data using public key of the user
	// under which the client claims to run
	const QByteArray sig = sdev.read().toByteArray();

	qDebug() << "Loading public key" << LocalSystem::Path::publicKeyPath( urole )
				<< "for role" << urole;
	// (publicKeyPath does range-checking of urole)
	PublicDSAKey pubKey( LocalSystem::Path::publicKeyPath( urole ) );

	return pubKey.verifySignature( chall, sig );
}




bool ItalcCoreServer::doHostBasedAuth( const QString &host )
{
	QMutexLocker l( &m_dataMutex );

	qDebug() << "ItalcCoreServer: doing host based auth for host" << host;

	if( m_allowedIPs.isEmpty() )
	{
		qWarning() << "ItalcCoreServer: empty list of allowed IPs";
		return false;
	}

	// already valid IP?
	if( QHostAddress().setAddress( host ) )
	{
		if( m_allowedIPs.contains( host ) )
		{
			return true;
		}
	}
	else
	{
		// create a list of all known addresses of host
		QList<QHostAddress> addr = QHostInfo::fromName( host ).addresses();

		// check each address for existence in list of allowed clients
		foreach( const QHostAddress a, addr )
		{
			if( m_allowedIPs.contains( a.toString() ) ||
					a.toString() == QHostAddress( QHostAddress::LocalHost ).
																	toString() )
			{
				return true;
			}
		}
	}

	qWarning() << "ItalcCoreServer::doHostBasedAuth() failed for host " << host;

	// host-based authentication failed
	return false;
}




bool ItalcCoreServer::doCommonSecretAuth( SocketDevice &sdev )
{
	qDebug() << "ItalcCoreServer: doing common secret auth";

	const QString secret = sdev.read().toString();
	if( secret == ItalcCore::authenticationCredentials->commonSecret() )
	{
		return true;
	}

	return false;
}

