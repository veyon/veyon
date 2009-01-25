/*
 * italc_core_server.cpp - ItalcCoreServer
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
ItalcCoreServer::GuiOpsMap ItalcCoreServer::guiOps;



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
	guiOps["fullscreendemo"] = CommandProperties( ItalcCore::StartFullScreenDemo, true, true );
	guiOps["windowdemo"] = CommandProperties( ItalcCore::StartWindowDemo, true, true );
	guiOps["stopdemo"] = CommandProperties( ItalcCore::StopDemo, true, true );
	guiOps["lockdisplay"] = CommandProperties( ItalcCore::LockDisplay, false, true );
	guiOps["unlockdisplay"] = CommandProperties( ItalcCore::UnlockDisplay, false, true );
	guiOps["displaytextmessage"] = CommandProperties( ItalcCore::DisplayTextMessage, true, true );
	guiOps["accessdialog"] = CommandProperties( ItalcCore::AccessDialog, true, false );
}





ItalcCoreServer::ItalcCoreServer( int _argc, char * * _argv ) :
	QObject(),
	m_ivs( NULL ),
	m_demoClient( NULL ),
	m_lockWidget( NULL )
{
	Q_ASSERT( _this == NULL );
	_this = this;

	for( int i = 0; i < NumOfGuiProcs; ++i )
	{
		m_guiProcs[i] = NULL;
	}

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
	char cmd;
	if( sdev.read( &cmd, sizeof( cmd ) ) == 0 )
	{
		qCritical( "ItalcCoreServer::processClient(...): couldn't read "
					"command from client..." );
		return false;
	}

	// in every case receive message-arguments, even if it's an empty list
	// because this is at leat the int32 with number of items in the list
	ItalcCore::Msg msg_in( &sdev, static_cast<ItalcCore::Commands>( cmd ) );
	msg_in.receive();

	QString action;

	switch( cmd )
	{
		case ItalcCore::GetUserInformation:
		{
			ItalcCore::Msg( &sdev, ItalcCore::UserInformation ).
					addArg( "username",
						localSystem::currentUser() ).
					addArg( "homedir", QDir::homePath() ).
									send();
			break;
		}

		case ItalcCore::ExecCmds:
		{
			const QString cmds = msg_in.arg( "cmds" ).toString();
			if( !cmds.isEmpty() )
			{
#ifdef ITALC_BUILD_WIN32
	// run process as the user which is logged on
	DWORD aProcesses[1024], cbNeeded;

	if( !EnumProcesses( aProcesses, sizeof( aProcesses ), &cbNeeded ) )
	{
		break;
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
		break;
	}
#else
				QProcess::startDetached( cmds );
#endif
			}
			break;
		}

		case ItalcCore::StartFullScreenDemo:
		case ItalcCore::StartWindowDemo:
		{
			QString port = msg_in.arg( "port" ).toString();
			if( port == "" )
			{
				port = "5858";
			}
			if( !port.contains( ':' ) )
			{
				const int MAX_HOST_LEN = 255;
				char host[MAX_HOST_LEN+1];
				_sd( host, MAX_HOST_LEN, SocketGetPeerAddress,
									_user );
				host[MAX_HOST_LEN] = 0;
				action = host + QString( ":" ) + port;
			}
			else
			{
				action = port;
			}
			break;
		}

		case ItalcCore::DisplayTextMessage:
			action = msg_in.arg( "msg" ).toString();
			break;

		case ItalcCore::LockDisplay:
		case ItalcCore::UnlockDisplay:
		case ItalcCore::StopDemo:
			action = "123";	// something to make the action being
					// added to action-list processed by
					// GUI-thread
			break;

		case ItalcCore::LogonUserCmd:
			localSystem::logonUser(
					msg_in.arg( "uname" ).toString(),
					msg_in.arg( "passwd" ).toString(),
					msg_in.arg( "domain" ).toString() );
			break;

		case ItalcCore::LogoutUser:
			localSystem::logoutUser();
			break;

		case ItalcCore::WakeOtherComputer:
			localSystem::broadcastWOLPacket( 
					msg_in.arg( "mac" ).toString() );
			break;

		case ItalcCore::PowerDownComputer:
			localSystem::powerDown();
			break;

		case ItalcCore::RestartComputer:
			localSystem::reboot();
			break;

		case ItalcCore::DisableLocalInputs:
			localSystem::disableLocalInputs(
					msg_in.arg( "disabled" ).toBool() );
			break;

		case ItalcCore::SetRole:
		{
			const int role = msg_in.arg( "role" ).toInt();
			if( role > ItalcCore::RoleNone && role < ItalcCore::RoleCount )
			{
				ItalcCore::role = static_cast<ItalcCore::UserRoles>( role );
#ifdef ITALC_BUILD_LINUX
				// under Linux/X11, IVS runs in separate process
				// therefore we need to restart it with new
				// role, bad hack but there's no clean solution
				// for the time being
				m_ivs->restart();
#endif
			}
			break;
		}

		default:
			qCritical( "ItalcCoreServer::processClient(...): "
					"cmd %d not implemented!", cmd );
			break;
	}

	if( !action.isEmpty() )
	{
		m_actionMutex.lock();
		m_pendingActions.push_back( qMakePair(
				static_cast<ItalcCore::Commands>( cmd ), action ) );
		m_actionMutex.unlock();
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
				if( __denied_hosts.contains( host ) )
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
				else
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




int ItalcCoreServer::doGuiOp( ItalcCore::Commands _cmd, const QString & _arg )
{
	bool extProc = ( _this != NULL );

	if( extProc )
	{
		QString procArg;
		bool runDetached = false;
		foreach( const QString & str, guiOps.keys() )
		{
			if( guiOps[str].cmd == _cmd )
			{
				runDetached = guiOps[str].runDetached;
				procArg = str;
				break;
			}
		}
		if( procArg.isEmpty() )
		{
			qCritical( "ItalcCoreServer::doGuiOp(): unhandled "
							"ItalcCore::Command" );
			return -1;
		}

		if( runDetached )
		{
			QProcess * p = new QProcess( _this );

			// kill old processes
			switch( _cmd )
			{
				case ItalcCore::StartWindowDemo:
				case ItalcCore::StartFullScreenDemo:
				case ItalcCore::StopDemo:
					if( _this->m_guiProcs[ProcDemo] )
					{
						_this->m_guiProcs[ProcDemo]->terminate();
						delete _this->m_guiProcs[ProcDemo];
						_this->m_guiProcs[ProcDemo] = NULL;
					}
					break;

				case ItalcCore::LockDisplay:
				case ItalcCore::UnlockDisplay:
					if( _this->m_guiProcs[ProcDisplayLock] )
					{
						_this->m_guiProcs[ProcDisplayLock]->terminate();
						delete _this->m_guiProcs[ProcDisplayLock];
						_this->m_guiProcs[ProcDisplayLock] = NULL;
					}
					break;

				default:
					break;
			}

			// save pointer to new process or return
			switch( _cmd )
			{
				case ItalcCore::StartWindowDemo:
				case ItalcCore::StartFullScreenDemo:
					_this->m_guiProcs[ProcDemo] = p;
					break;

				case ItalcCore::LockDisplay:
					_this->m_guiProcs[ProcDisplayLock] = p;
					break;

				default:
					return 0;
			}
			p->start( QCoreApplication::applicationFilePath(),
					QStringList() << procArg << _arg );
			return 0;
		}
		else
		{
			return QProcess::execute(
				QCoreApplication::applicationFilePath(),
					QStringList() << procArg << _arg );
		}
	}

	switch( _cmd )
	{
		case ItalcCore::AccessDialog:
			return _this->showAccessDialog( _arg );
			break;

		case ItalcCore::StartFullScreenDemo:
		case ItalcCore::StartWindowDemo:
			_this->startDemo( _arg, _cmd == ItalcCore::StartFullScreenDemo );
			break;

		case ItalcCore::LockDisplay:
			_this->lockDisplay();
			break;

		case ItalcCore::DisplayTextMessage:
			_this->displayTextMessage( _arg );
			break;

		default:
			qWarning( "ItalcCoreServer::doGuiOp():"
					" unhandled command %d", (int) _cmd );
			break;
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
		QString data = m_pendingActions.front().second;
		doGuiOp( m_pendingActions.front().first, data );
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




