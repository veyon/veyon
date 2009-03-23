/*
 * italc_core_server.cpp - ItalcCoreServer
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include <QtCore/QProcess>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QTcpSocket>

#include "italc_core_server.h"
#include "dsa_key.h"
#include "local_system_ica.h"
#include "ivs.h"
#include "lock_widget.h"
#include "messagebox.h"
#include "demo_client.h"
#include "ica_main.h"


ItalcCoreServer * ItalcCoreServer::_this = NULL;
QList<ItalcCore::Command> ItalcCoreServer::externalActions;



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




void ItalcCoreServer::earlyInit( void )
{
	externalActions << ItalcCore::StartDemo
			<< ItalcCore::StopDemo
			<< ItalcCore::DisplayTextMessage
			<< ItalcCore::LockDisplay
			<< ItalcCore::UnlockDisplay;

	// TODO: init pugins
}





ItalcCoreServer::ItalcCoreServer( int _argc, char * * _argv ) :
	QObject(),
	m_ivs( NULL ),
	m_demoClient( NULL ),
	m_lockWidget( NULL )
{
	Q_ASSERT( _this == NULL );
	_this = this;

	QTimer * t = new QTimer( this );
	connect( t, SIGNAL( timeout() ), this,
					SLOT( checkForPendingActions() ) );
	// as things like creating a demo-window, remote-control-view etc. can
	// only be done by GUI-thread we push all actions into a list and
	// process this list later in a slot called by the GUI-thread every 500s
	t->start( 300 );

	m_ivs = new IVS( __ivs_port, _argc, _argv );
	m_ivs->start(/* QThread::HighPriority*/ );
}




ItalcCoreServer::~ItalcCoreServer()
{
	delete m_ivs;
	delete m_lockWidget;
	_this = NULL;
}




int ItalcCoreServer::processClient( socketDispatcher _sd, void * _user )
{
	SocketDevice sdev( _sd, _user );

	// receive message
	ItalcCore::Msg msgIn( &sdev );
	msgIn.receive();

	const QString cmd = msgIn.cmd();
	if( cmd == ItalcCore::GetUserInformation )
	{
		ItalcCore::Msg( &sdev, ItalcCore::UserInformation ).
					addArg( "username",
						localSystem::currentUser() ).
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
	else if( cmd == ItalcCore::LogonUser )
	{
		localSystem::logonUser( msgIn.arg( "uname" ),
					msgIn.arg( "passwd" ),
					msgIn.arg( "domain" ) );
	}
	else if( cmd == ItalcCore::LogoutUser )
	{
		localSystem::logoutUser();
	}
	else if( cmd == ItalcCore::WakeOtherComputer )
	{
		localSystem::broadcastWOLPacket( msgIn.arg( "mac" ) );
	}
	else if( cmd == ItalcCore::PowerDownComputer )
	{
		localSystem::powerDown();
	}
	else if( cmd == ItalcCore::RestartComputer )
	{
		localSystem::reboot();
	}
	else if( cmd == ItalcCore::DisableLocalInputs )
	{
		localSystem::disableLocalInputs(
					msgIn.arg( "disabled" ).toInt() );
	}
	else if( cmd == ItalcCore::SetRole )
	{
		const int role = msgIn.arg( "role" ).toInt();
		if( role > ItalcCore::RoleNone && role < ItalcCore::RoleCount )
		{
			ItalcCore::role =
				static_cast<ItalcCore::UserRoles>( role );
#ifdef ITALC_BUILD_LINUX
				// under Linux/X11, IVS runs in separate process
				// therefore we need to restart it with new
				// role, bad hack but there's no clean solution
				// for the time being
				m_ivs->restart();
#endif
		}
	}
	else if( cmd == ItalcCore::StartDemo ||
			cmd == ItalcCore::StopDemo ||
			cmd == ItalcCore::DisplayTextMessage ||
			cmd == ItalcCore::LockDisplay ||
			cmd == ItalcCore::UnlockDisplay )
	{
		// edit arguments if neccessary
		ItalcCore::CommandArgs args = msgIn.args();
		if( cmd == ItalcCore::StartDemo )
		{
			QString host;
			QString port = args["port"].toString();
			if( port == "" )
			{
				port = "5858";
			}
			if( !port.contains( ':' ) )
			{
				const int MAX_HOST_LEN = 255;
				char hostArr[MAX_HOST_LEN+1];
				_sd( hostArr, MAX_HOST_LEN,
					SocketGetPeerAddress, _user );
				hostArr[MAX_HOST_LEN] = 0;
				host = hostArr + QString( ":" ) + port;
			}
			else
			{
				host = port;
			}
			args["host"] = host;
		}
		m_actionMutex.lock();
		m_pendingActions.push_back( qMakePair( cmd, args ) );
		m_actionMutex.unlock();
	}
	// TODO: handle plugins
	else
	{
		qCritical() << "ItalcCoreServer::processClient(...): "
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

	ItalcAuthResults result = ItalcAuthFailed;

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
printf("auth sec %d\n", (int)_auth_type );

	switch( _auth_type )
	{
		// no authentication
		case ItalcAuthNone:
			result = ItalcAuthOK;
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
					result = ItalcAuthOK;
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
						result = ItalcAuthOK;
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
			const QByteArray chall = dsaKey::generateChallenge();
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
					result = ItalcAuthFailed;
				}
			}
			
			// now try to verify received signed data using public
			// key of the user under which the client claims to run
			const QByteArray sig = sdev.read().toByteArray();
			// (publicKeyPath does range-checking of urole)
			publicDSAKey pub_key( localSystem::publicKeyPath(
								urole ) );
			result = pub_key.verifySignature( chall, sig ) ?
						ItalcAuthOK : ItalcAuthFailed;
			break;
		}
	}
printf("dsa: %d\n", (int)result);

	sdev.write( QVariant( (int) result ) );
	if( result != ItalcAuthOK )
	{
		errorMsgAuth( host );
	}

	return( result == ItalcAuthOK );
}




int ItalcCoreServer::doGuiOp( const ItalcCore::Command & _cmd,
					const ItalcCore::CommandArgs & _args )
{
	bool extProc = ( _this != NULL );

	if( extProc )
	{
		bool runDetached = ( _cmd == ItalcCore::AccessDialog ) ?
								false : true;
		QString procArgs = _cmd;
		for( ItalcCore::CommandArgs::ConstIterator it = _args.begin();
						it != _args.end(); ++it )
		{
			procArgs += "," + it.key() + "=" +
							it.value().toString();
		}
		if( runDetached )
		{
			QProcess * p = new QProcess( _this );
			if( _this->m_guiProcs.contains( _cmd ) )
			{
				QProcess * oldProc = _this->m_guiProcs[_cmd];
				if( oldProc )
				{
					oldProc->terminate();
					delete oldProc;
					_this->m_guiProcs[_cmd] = NULL;
				}
			}

			_this->m_guiProcs[_cmd] = p;
			p->start( QCoreApplication::applicationFilePath(),
					QStringList() << procArgs );
			return 0;
		}
		else
		{
			return QProcess::execute(
				QCoreApplication::applicationFilePath(),
					QStringList() << procArgs );
		}
	}

	if( _cmd == ItalcCore::AccessDialog )
	{
		return _this->showAccessDialog( _args["host"].toString() );
	}
	else if( _cmd == ItalcCore::StartDemo )
	{
		_this->startDemo( _args["host"].toString(),
					_args["fullscreen"].toInt() );
	}
	else if( _cmd == ItalcCore::LockDisplay )
	{
		_this->lockDisplay();
	}
	else if( _cmd == ItalcCore::DisplayTextMessage )
	{
		_this->displayTextMessage( _args["text"].toString() );
	}
	else
	{
		// TODO: handle plugins
		qWarning() << "ItalcCoreServer::doGuiOp():"
				" unhandled command" << _cmd;
	}
	return 0;
}




ItalcCoreServer::AccessDialogResult ItalcCoreServer::showAccessDialog(
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

	localSystem::activateWindow( &m );

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
}




void ItalcCoreServer::checkForPendingActions( void )
{
	QMutexLocker ml( &m_actionMutex );
	while( !m_pendingActions.isEmpty() )
	{
		doGuiOp( m_pendingActions.front().first,
				m_pendingActions.front().second );
		m_pendingActions.removeFirst();
	}
}




void ItalcCoreServer::startDemo( const QString & _master_host, bool _fullscreen )
{
	delete m_demoClient;
	m_demoClient = NULL;
	// if a demo-server is started, it's likely that the demo was started
	// on master-computer as well therefore we deny starting a demo on
	// hosts on which a demo-server is running
/*	if( demoServer::numOfInstances() > 0 )
	{
		return;
	}*/

	m_demoClient = new DemoClient( _master_host, _fullscreen );
}




void ItalcCoreServer::stopDemo( void )
{
	delete m_demoClient;
	m_demoClient = NULL;
}




void ItalcCoreServer::lockDisplay( void )
{
/*	if( demoServer::numOfInstances() )
	{
		return;
	}*/
	delete m_lockWidget;
	m_lockWidget = new LockWidget();
}




void ItalcCoreServer::unlockDisplay( void )
{
	delete m_lockWidget;
	m_lockWidget = NULL;
}




void ItalcCoreServer::displayTextMessage( const QString & _msg )
{
	new messageBox( tr( "Message from teacher" ), _msg,
					QPixmap( ":/resources/message.png" ) );
}




void ItalcCoreServer::errorMsgAuth( const QString & _ip )
{
	messageBox::trySysTrayMessage( tr( "Authentication error" ),
			tr( "Somebody (IP: %1) tried to access this computer "
					"but could not authenticate itself "
					"successfully!" ).arg( QString( _ip ) ),
						messageBox::Critical );
}




