/*
 * ItalcCoreServer.cpp - ItalcCoreServer
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <psapi.h>
#endif


#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
//#include <QtCore/QTemporaryFile>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QTcpSocket>

#include "ItalcCoreServer.h"
#include "DsaKey.h"
#include "LocalSystemIca.h"
#include "IcaMain.h"


ItalcCoreServer * ItalcCoreServer::_this = NULL;



qint64 qtcpsocketDispatcher( char * _buf, const qint64 _len,
				const SocketOpCodes _op_code, void * _user )
{
	QTcpSocket * sock = static_cast<QTcpSocket *>( _user );
	qint64 ret = 0;
	switch( _op_code )
	{
		case SocketRead:
			while( ret < _len )
			{
				qint64 bytes_read = sock->read( _buf, _len );
				if( bytes_read < 0 )
				{
	qDebug( "qtcpsocketDispatcher(...): connection closed while reading" );
					return 0;
				}
				else if( bytes_read == 0 )
				{
					if( sock->state() !=
						QTcpSocket::ConnectedState )
					{
	qDebug( "qtcpsocketDispatcher(...): connection failed while writing  "
			"state:%d  error:%d", sock->state(), sock->error() );
						return 0;
					}
					sock->waitForReadyRead( 10 );
				}
				ret += bytes_read;
			}
			break;
		case SocketWrite:
			while( ret < _len )
			{
				qint64 written = sock->write( _buf, _len );
				if( written < 0 )
				{
	qDebug( "qtcpsocketDispatcher(...): connection closed while writing" );
					return 0;
				}
				else if( written == 0 )
				{
					if( sock->state() !=
						QTcpSocket::ConnectedState )
					{
	qDebug( "qtcpsocketDispatcher(...): connection failed while writing  "
			"state:%d error:%d", sock->state(), sock->error() );
						return 0;
					}
				}
				ret += written;
			}
			//sock->flush();
			sock->waitForBytesWritten();
			break;
		case SocketGetPeerAddress:
			strncpy( _buf,
		sock->peerAddress().toString().toAscii().constData(), _len );
			break;
	}
	return ret;
}




ItalcCoreServer::ItalcCoreServer() :
	QObject(),
	m_masterProcess()
{
	Q_ASSERT( _this == NULL );
	_this = this;
}




ItalcCoreServer::~ItalcCoreServer()
{
	_this = NULL;
}




int ItalcCoreServer::handleItalcClientMessage( socketDispatcher sock,
												void *user )
{
	SocketDevice sdev( sock, user );

	// receive message
	ItalcCore::Msg msgIn( &sdev );
	msgIn.receive();

	const QString cmd = msgIn.cmd();
	if( cmd == ItalcCore::GetUserInformation )
	{
		ItalcCore::Msg( &sdev, ItalcCore::UserInformation ).
					addArg( "username",
						LocalSystem::currentUser() ).
					addArg( "homedir", QDir::homePath() ).
									send();
	}
	else if( cmd == ItalcCore::ExecCmds )
	{
		const QString cmds = msgIn.arg( "cmds" );
		if( !cmds.isEmpty() )
		{
#ifdef ITALC_BUILD_WIN32
	// run process as the user which is logged on
	DWORD aProcesses[1024], cbNeeded;

	if( !EnumProcesses( aProcesses, sizeof( aProcesses ), &cbNeeded ) )
	{
		return false;
	}

	DWORD cProcesses = cbNeeded / sizeof(DWORD);

	for( DWORD i = 0; i < cProcesses; i++ )
	{
		HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS,
							false, aProcesses[i] );
		HMODULE hMod;
		if( hProcess == NULL ||
			!EnumProcessModules( hProcess, &hMod, sizeof( hMod ),
								&cbNeeded ) )
	        {
			continue;
		}

		TCHAR szProcessName[MAX_PATH];
		GetModuleBaseName( hProcess, hMod, szProcessName,
				sizeof( szProcessName ) / sizeof( TCHAR) );
		for( TCHAR * ptr = szProcessName; *ptr; ++ptr )
		{
			*ptr = tolower( *ptr );
		}

		if( strcmp( szProcessName, "explorer.exe" ) )
		{
			CloseHandle( hProcess );
			continue;
		}

		HANDLE hToken;
		OpenProcessToken( hProcess, MAXIMUM_ALLOWED, &hToken );
		ImpersonateLoggedOnUser( hToken );

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory( &si, sizeof( STARTUPINFO ) );
		si.cb= sizeof( STARTUPINFO );
		si.lpDesktop = (CHAR *) "winsta0\\default";
		HANDLE hNewToken = NULL;

		DuplicateTokenEx( hToken, MAXIMUM_ALLOWED, NULL,
					SecurityImpersonation, TokenPrimary,
								&hNewToken );

		CreateProcessAsUser(
				hNewToken,            // client's access token
				NULL,              // file to execute
				(CHAR *)cmds.toAscii().constData(),     // command line
				NULL,              // pointer to process SECURITY_ATTRIBUTES
				NULL,              // pointer to thread SECURITY_ATTRIBUTES
				false,             // handles are not inheritable
				NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,   // creation flags
				NULL,              // pointer to new environment block
				NULL,              // name of current directory
				&si,               // pointer to STARTUPINFO structure
				&pi                // receives information about new process
				);
		CloseHandle( hNewToken );
		RevertToSelf();
		CloseHandle( hToken );
		CloseHandle( hProcess );
	}
#else
				QProcess::startDetached( cmds );
#endif
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
	else if( cmd == ItalcCore::PowerOnComputer )
	{
		LocalSystem::broadcastWOLPacket( msgIn.arg( "mac" ) );
	}
	else if( cmd == ItalcCore::PowerDownComputer )
	{
		LocalSystem::powerDown();
	}
	else if( cmd == ItalcCore::RestartComputer )
	{
		LocalSystem::reboot();
	}
	else if( cmd == ItalcCore::DisableLocalInputs )
	{
		LocalSystem::disableLocalInputs( msgIn.arg( "disabled" ).toInt() );
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
		QString host;
		QString port = msgIn.arg( "port" );
		if( port.isEmpty() )
		{
			port = "11200";	// TODO: convert from global constant
		}
		if( !port.contains( ':' ) )
		{
			const int MAX_HOST_LEN = 255;
			char hostArr[MAX_HOST_LEN+1];
			sock( hostArr, MAX_HOST_LEN,
					SocketGetPeerAddress, user );
			hostArr[MAX_HOST_LEN] = 0;
			host = hostArr + QString( ":" ) + port;
		}
		else
		{
			host = port;
		}
		m_masterProcess.startDemo( host, msgIn.arg( "fullscreen" ).toInt() );
	}
	else if( cmd == ItalcCore::StopDemo )
	{
		m_masterProcess.stopDemo();
	}
	else if( cmd == ItalcCore::DisplayTextMessage )
	{
		m_masterProcess.messageBox( msgIn.arg( "text" ) );
	}
	else if( cmd == ItalcCore::LockDisplay )
	{
		m_masterProcess.lockDisplay();
	}
	else if( cmd == ItalcCore::UnlockDisplay )
	{
		m_masterProcess.unlockDisplay();
	}
	// TODO: handle plugins
	else
	{
		qCritical() << "ItalcCoreServer::handleItalcClientMessage(...): "
				"could not handle cmd" << cmd;
	}

	return true;
}




bool ItalcCoreServer::authSecTypeItalc( socketDispatcher _sd, void * _user,
					ItalcAuthTypes _auth_type,
					const QStringList & _allowedHosts )
{
	// find out IP of host - needed at several places
	const int MAX_HOST_LEN = 255;
	char host[MAX_HOST_LEN+1];
	_sd( host, MAX_HOST_LEN, SocketGetPeerAddress, _user );
	host[MAX_HOST_LEN] = 0;
	static QStringList __denied_hosts, __allowed_hosts;

	SocketDevice sdev( _sd, _user );
	sdev.write( QVariant( (int) _auth_type ) );

	uint32_t result = rfbVncAuthFailed;

	ItalcAuthTypes chosen = static_cast<ItalcAuthTypes>(
							sdev.read().toInt() );
	if( chosen == ItalcAuthDSA && _auth_type == ItalcAuthLocalDSA )
	{
		// this case is ok as well
	}
	else if( chosen != _auth_type )
	{
		errorMsgAuth( host );
		qCritical( "ItalcCoreServer::authSecTypeItalc(...): "
				"client chose other auth-type than offered!" );
		return( result );
	}

	switch( _auth_type )
	{
		// no authentication
		case ItalcAuthNone:
			result = rfbVncAuthOK;
			break;

		// host has to be in list of allowed hosts
		case ItalcAuthHostBased:
		{
			if( _allowedHosts.isEmpty() )
			{
				break;
			}
			QStringList allowed;
			foreach( const QString a, _allowedHosts )
			{
				const QString h = a.split( ':' )[0];
				if( !allowed.contains( h ) )
				{
					allowed.push_back( h );
				}
			}
			// already valid IP?
			if( QHostAddress().setAddress( host ) )
			{
				if( allowed.contains( host ) )
				{
					result = rfbVncAuthOK;
				}
			}
			else
			{
			// create a list of all known addresses of host
			QList<QHostAddress> addr =
					QHostInfo::fromName( host ).addresses();
			if( !addr.isEmpty() )
			{
				// check each address for existence in allowed-
				// client-list
				foreach( const QHostAddress a, addr )
				{
	if( allowed.contains( a.toString() ) ||
		a.toString() == QHostAddress( QHostAddress::LocalHost ).toString() )
					{
						result = rfbVncAuthOK;
						break;
					}
				}
			}
			}
			break;
		}

		// authentication via DSA-challenge/-response
		case ItalcAuthLocalDSA:
		case ItalcAuthDSA:
		{
			// generate data to sign and send to client
			const QByteArray chall = DsaKey::generateChallenge();
			sdev.write( QVariant( chall ) );

			// get user-role
			const ItalcCore::UserRoles urole =
				static_cast<ItalcCore::UserRoles>(
							sdev.read().toInt() );
			if( ItalcCore::role != ItalcCore::RoleOther &&
					_auth_type != ItalcAuthLocalDSA )
			{
			/*	if( __denied_hosts.contains( host ) )
				{
					result = ItalcAuthFailed;
					break;
				}
				if( !__allowed_hosts.contains( host ) )
				{
					bool failed = true;
					switch( doGuiOp( ItalcCore::AccessDialog, host ) )
					{
						case AccessAlways:
							__allowed_hosts += host;
						case AccessYes:
							failed = false;
							break;
						case AccessNever:
							__denied_hosts += host;
						case AccessNo:
							break;
					}
					if( failed )
					{
						result = ItalcAuthFailed;
						break;
					}
				}
				else*/
				{
					result = rfbVncAuthFailed;
				}
			}

			// now try to verify received signed data using public
			// key of the user under which the client claims to run
			const QByteArray sig = sdev.read().toByteArray();
			// (publicKeyPath does range-checking of urole)
			PublicDSAKey pub_key( LocalSystem::publicKeyPath(
								urole ) );
			result = pub_key.verifySignature( chall, sig ) ?
						rfbVncAuthOK : rfbVncAuthFailed;
			break;
		}
	}

	if( result != rfbVncAuthOK )
	{
		errorMsgAuth( host );
		return false;
	}

	return true;
}




/*ItalcCoreServer::AccessDialogResult ItalcCoreServer::showAccessDialog(
							const QString & _host )
{
	QMessageBox m( QMessageBox::Question,
			tr( "Confirm access" ),
			tr( "Somebody at host %1 tries to access your screen. "
				"Do you want to grant him/her access?" ).
								arg( _host ),
				QMessageBox::Yes | QMessageBox::No );

	QPushButton * never_btn = m.addButton( tr( "Never for this session" ),
							QMessageBox::NoRole );
	QPushButton * always_btn = m.addButton( tr( "Always for this session" ),
							QMessageBox::YesRole );
	m.setDefaultButton( never_btn );
	m.setEscapeButton( m.button( QMessageBox::No ) );

	LocalSystem::activateWindow( &m );

	const int res = m.exec();
	if( m.clickedButton() == never_btn )
	{
		return AccessNever;
	}
	else if( m.clickedButton() == always_btn )
	{
		return AccessAlways;
	}
	else if( res == QMessageBox::No )
	{
		return AccessNo;
	}
	return AccessYes;
}*/




/*void ItalcCoreServer::displayTextMessage( const QString & _msg )
{
	new DecoratedMessageBox( tr( "Message from teacher" ), _msg,
					QPixmap( ":/resources/message.png" ) );
}*/




void ItalcCoreServer::errorMsgAuth( const QString &ip )
{
	_this->m_masterProcess.systemTrayMessage(
			tr( "Authentication error" ),
			tr( "Somebody (IP: %1) tried to access this computer "
					"but could not authenticate itself "
					"successfully!" ).arg( ip ) );
}



