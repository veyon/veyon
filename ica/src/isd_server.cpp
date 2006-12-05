/*
 * isd_server.cpp - ISD Server
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>
#include <QtGui/QMessageBox>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QTcpSocket>

#include "isd_server.h"
#include "isd_connection.h"
#include "dsa_key.h"
#include "local_system.h"
#include "remote_control_widget.h"
#include "ivs.h"
#include "lock_widget.h"
#include "messagebox.h"
#include "demo_client.h"
#include "demo_server.h"
#include "ica_main.h"
#include "qt_features.h"

#ifdef SYSTEMTRAY_SUPPORT
#include <QtGui/QSystemTrayIcon>
#endif


static isdServer * __isd_server = NULL;

QByteArray isdServer::s_appInternalChallenge;
QStringList isdServer::s_allowedDemoClients;



isdServer::isdServer( const quint16 _ivs_port, int _argc, char * * _argv ) :
	QTcpServer(),
	m_ivs( new IVS( _ivs_port, _argc, _argv ) ),
	m_demoClient( NULL ),
	m_demoServer( NULL ),
	m_lockWidget( NULL )
{
	if( __isd_server ||
			listen( QHostAddress::LocalHost, __isd_port ) == FALSE )
	{
		// uh oh, already an ISD running or port isn't available...
		printf( "Could not start ISD server: %s\n",
					errorString().toAscii().constData() );
	}

	m_ivs->start(/* QThread::HighPriority*/ );

	connect( this, SIGNAL( newConnection() ),
			this, SLOT( acceptNewConnection() ) );

	QTimer * t = new QTimer( this );
	connect( t, SIGNAL( timeout() ), this,
					SLOT( checkForPendingActions() ) );
	// as things like creating a demo-window, remote-control-view etc. can
	// only be done by GUI-thread we push all actions into a list and
	// process this list later in a slot called by the GUI-thread every 500s
	t->start( 300 );

	// finally we set the global pointer to ourself
	__isd_server = this;
}




isdServer::~isdServer()
{
	delete m_ivs;
	delete m_demoServer;
	delete m_lockWidget;
	__isd_server = NULL;
}




int isdServer::processClient( socketDispatcher _sd, void * _user )
{
	socketDevice sdev( _sd, _user );
	char cmd;
	if( sdev.read( &cmd, sizeof( cmd ) ) == 0 )
	{
		printf( "couldn't read iTALC-request from client...\n" );
		return( FALSE );
	}

	if( cmd == rfbItalcServiceRequest )
	{
		return( processClient( _sd, _user ) );
	}

	// in every case receive message-arguments, even if it's an empty list
	// because this is at leat the int32 with number of items in the list
	ISD::msg msg_in( &sdev, static_cast<ISD::commands>( cmd ) );
	msg_in.receive();

	QString action;

	switch( cmd )
	{
		case ISD::GetUserInformation:
		{
			ISD::msg( &sdev, ISD::UserInformation ).
					addArg( "username",
						localSystem::currentUser() ).
					addArg( "homedir", QDir::homePath() ).
									send();
			break;
		}

		case ISD::ExecCmds:
		{
			const QString cmds = msg_in.arg( "cmds" ).toString();
			if( !cmds.isEmpty() )
			{
				QProcess::execute( cmds );
			}
			break;
		}

		case ISD::StartFullScreenDemo:
		case ISD::StartWindowDemo:
		case ISD::ViewRemoteDisplay:
		case ISD::RemoteControlDisplay:
			action = msg_in.arg( "ip" ).toString();
			break;

		case ISD::DisplayTextMessage:
			action = msg_in.arg( "msg" ).toString();
			break;

		case ISD::LockDisplay:
		case ISD::UnlockDisplay:
		case ISD::StopDemo:
			action = "123";	// something to make the action being
					// added to action-list processed by
					// GUI-thread
			break;

		case ISD::LogonUserCmd:
			localSystem::logonUser(
					msg_in.arg( "uname" ).toString(),
					msg_in.arg( "passwd" ).toString(),
					msg_in.arg( "domain" ).toString() );
			break;

		case ISD::LogoutUser:
			localSystem::logoutUser();
			break;

		case ISD::WakeOtherComputer:
			localSystem::broadcastWOLPacket( 
					msg_in.arg( "mac" ).toString() );
			break;

		case ISD::PowerDownComputer:
			localSystem::powerDown();
			break;

		case ISD::RestartComputer:
			localSystem::reboot();
			break;

		case ISD::SetRole:
		{
			const int role = msg_in.arg( "role" ).toInt();
			if( role > ISD::RoleNone && role < ISD::RoleCount )
			{
				__role = static_cast<ISD::userRoles>( role );
#ifdef BUILD_LINUX
				// under Linux/X11, IVS runs in separate process
				// therefore we need to restart it with new
				// role, bad hack but there's no clean solution
				// for the time being
				m_ivs->restart();
#endif
			}
			break;
		}

		case ISD::DemoServer_Run:
			// make sure any running demo-server is destroyed
			delete m_demoServer;
			// start demo-server on local IVS
			m_demoServer = new demoServer( m_ivs,
					msg_in.arg( "quality" ).toInt(),
					msg_in.arg( "port" ).toInt() );
			if( _sd == &qtcpsocketDispatcher )
			{
				QTcpSocket * ts = static_cast<QTcpSocket *>(
								_user );
				ts->setProperty( "demoserver",
					QVariant::fromValue( (QObject *)
							m_demoServer ) );
			}
			break;

		case ISD::DemoServer_Stop:
		{
			const QList<QTcpSocket *> sockets =
						findChildren<QTcpSocket *>();
			foreach( QTcpSocket * _ts, sockets )
			{
				cleanupDestroyedConnection( _ts );
			}
			delete m_demoServer;
			m_demoServer = NULL;
			break;
		}

		case ISD::DemoServer_AllowClient:
			allowDemoClient( msg_in.arg( "client" ).toString() );
			break;

		case ISD::DemoServer_DenyClient:
			denyDemoClient( msg_in.arg( "client" ).toString() );
			break;

		default:
			printf( "cmd %d not implemented!\n", cmd );
			break;
	}

	if( !action.isEmpty() )
	{
		m_actionMutex.lock();
		m_pendingActions.push_back( qMakePair(
				static_cast<ISD::commands>( cmd ), action ) );
		m_actionMutex.unlock();
	}

	return( TRUE );
}




bool isdServer::protocolInitialization( socketDevice & _sd,
					italcAuthTypes _auth_type,
					bool _demo_server )
{
	if( _demo_server )
	{
		idsProtocolVersionMsg pv;
		sprintf( pv, idsProtocolVersionFormat, idsProtocolMajorVersion,
						idsProtocolMinorVersion );
		_sd.write( pv, sz_idsProtocolVersionMsg );

		idsProtocolVersionMsg pv_cl;
		_sd.read( pv_cl, sz_idsProtocolVersionMsg );
		pv_cl[sz_idsProtocolVersionMsg] = 0;
		if( memcmp( pv, pv_cl, sz_idsProtocolVersionMsg ) )
		{
			printf( "invalid client!\n" );
			return FALSE;
		}
	}
	else
	{
		isdProtocolVersionMsg pv;
		sprintf( pv, isdProtocolVersionFormat, isdProtocolMajorVersion,
						isdProtocolMinorVersion );
		_sd.write( pv, sz_isdProtocolVersionMsg );

		isdProtocolVersionMsg pv_cl;
		_sd.read( pv_cl, sz_isdProtocolVersionMsg );
		pv_cl[sz_isdProtocolVersionMsg] = 0;
		if( memcmp( pv, pv_cl, sz_isdProtocolVersionMsg ) )
		{
			printf( "invalid client!\n" );
			return FALSE;
		}
	}


	const char sec_type_list[2] = { 1, rfbSecTypeItalc } ;
	_sd.write( sec_type_list, sizeof( sec_type_list ) );

	Q_UINT8 chosen = 0;
	_sd.read( (char *) &chosen, sizeof( chosen ) );

	if( chosen != rfbSecTypeItalc )
	{
		printf( "client wants unknown security type %d\n", chosen );
		return( FALSE );
	}

	if( !authSecTypeItalc( _sd.sockDispatcher(), _sd.user(), _auth_type ) )
	{
		return( FALSE );
	}

	return( TRUE );
}




bool isdServer::authSecTypeItalc( socketDispatcher _sd, void * _user,
						italcAuthTypes _auth_type )
{
	// find out IP of host - needed at several places
	const int MAX_HOST_LEN = 255;
	char host[MAX_HOST_LEN];
	_sd( host, MAX_HOST_LEN, SocketGetIPBoundTo, _user );
	host[MAX_HOST_LEN] = 0;

	static QStringList __denied_hosts, __allowed_hosts;

	socketDevice sdev( _sd, _user );
	sdev.write( QVariant( (int) _auth_type ) );

	italcAuthResults result = ItalcAuthFailed;

	italcAuthTypes chosen = static_cast<italcAuthTypes>(
							sdev.read().toInt() );
	if( chosen == ItalcAuthAppInternalChallenge ||
		chosen == ItalcAuthChallengeViaAuthFile )
	{
		_auth_type = chosen;
	}
	else if( chosen == ItalcAuthDSA && _auth_type == ItalcAuthLocalDSA )
	{
		// this case is ok as well
	}
	else if( chosen != _auth_type )
	{
		printf( "Client chose other auth-type than offered!\n" );
		return( result );
	}

	switch( _auth_type )
	{
		// no authentication
		case ItalcAuthNone:
			result = ItalcAuthOK;
			break;

		// host has to be in list of allowed hosts
		case ItalcAuthHostBased:
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
					if( s_allowedDemoClients.contains(
								a.toString() ) )
					{
						result = ItalcAuthOK;
						break;
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
			const ISD::userRoles urole =
				static_cast<ISD::userRoles>(
							sdev.read().toInt() );
			if( __role != ISD::RoleOther &&
					_auth_type != ItalcAuthLocalDSA )
			{
				if( __denied_hosts.contains( host ) )
				{
					result = ItalcAuthFailed;
					break;
				}
				if( !__allowed_hosts.contains( host ) )
				{
					bool failed = TRUE;
					switch(
#ifdef BUILD_LINUX
	QProcess::execute( QCoreApplication::applicationFilePath() +
					QString( " %1 %2" ).
						arg( ACCESS_DIALOG_ARG ).
								arg( host ) )
#else
					showAccessDialog( host )
#endif
									)
					{
						case Always:
							__allowed_hosts += host;
						case Yes:
							failed = FALSE;
							break;
						case Never:
							__denied_hosts += host;
						case No:
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

		// used for demo-purposes (demo-server connects to local IVS)
		case ItalcAuthAppInternalChallenge:
		{
			// generate challenge
			s_appInternalChallenge = dsaKey::generateChallenge();
			sdev.write( QVariant() );
			// is our client able to read this byte-array? if so,
			// it's for sure running inside the same app
			result = ( sdev.read().toByteArray() ==
						s_appInternalChallenge ) ?
						ItalcAuthOK : ItalcAuthFailed;
			break;
		}

		// used for demo-purposes (demo-server connects to local IVS)
		case ItalcAuthChallengeViaAuthFile:
		{
			// generate challenge
			QByteArray chall = dsaKey::generateChallenge();
			QTemporaryFile tf;
			tf.setPermissions( QFile::ReadOwner |
							QFile::WriteOwner );
			tf.open();
			tf.write( chall );
			sdev.write( tf.fileName() );
			// is our client able to read the file? if so,
			// it's running as the same user as this piece of
			// code does so we can assume that it's our parent-
			// process
			result = ( sdev.read().toByteArray() == chall ) ?
						ItalcAuthOK : ItalcAuthFailed;
			break;
		}
	}

	sdev.write( QVariant( (int) result ) );

	return( result == ItalcAuthOK );
}




quint16 isdServer::isdPort( void )
{
	return( __isd_server ? __isd_server->serverPort() : PortOffsetISD );
}



isdServer::accessDialogResult isdServer::showAccessDialog(
							const QString & _host )
{
	QMessageBox m(
#ifdef QMESSAGEBOX_EXT_SUPPORT
			QMessageBox::Question,
#endif
			tr( "Confirm access" ),
			tr( "Somebody at host %1 tries to access your screen. "
				"Do you want to grant him/her access?" ).
								arg( _host ),
#ifdef QMESSAGEBOX_EXT_SUPPORT
				QMessageBox::Yes | QMessageBox::No
#else
			QMessageBox::Question,
			QMessageBox::Yes,
			QMessageBox::No | QMessageBox::Escape,
			QMessageBox::NoToAll | QMessageBox::Default
#endif
			);
#ifdef QMESSAGEBOX_EXT_SUPPORT
	QPushButton * never_btn = m.addButton( tr( "Never for this session" ),
							QMessageBox::NoRole );
	QPushButton * always_btn = m.addButton( tr( "Always for this session" ),
							QMessageBox::YesRole );
	m.setDefaultButton( never_btn );
	m.setEscapeButton( m.button( QMessageBox::No ) );
#else
	m.setButtonText( 2, tr( "Never for this session" ) );
#endif
	m.activateWindow();

	const int res = m.exec();
#ifdef QMESSAGEBOX_EXT_SUPPORT
	if( m.clickedButton() == never_btn )
#else
	if( res == QMessageBox::NoToAll )
#endif
	{
		return( Never );
	}
#ifdef QMESSAGEBOX_EXT_SUPPORT
	else if( m.clickedButton() == always_btn )
	{
		return( Always );
	}
#endif
	else if( res == QMessageBox::No )
	{
		return( No );
	}
	return( Yes );
}



void isdServer::acceptNewConnection( void )
{
	QTcpSocket * sock = nextPendingConnection();
	socketDevice sd( qtcpsocketDispatcher, sock );

	if( !protocolInitialization( sd, ItalcAuthLocalDSA) )
	{
		delete sock;
		return;
	}

	// now we're ready to start the normal interaction with the client,
	// so make sure, we get informed about new requests
	connect( sock, SIGNAL( disconnected() ), sock, SLOT( deleteLater() ) );
	connect( sock, SIGNAL( readyRead() ), this, SLOT( processClients() ) );
	connect( sock, SIGNAL( destroyed( QObject * ) ), this,
			SLOT( cleanupDestroyedConnection( QObject * ) ) );
}




void isdServer::cleanupDestroyedConnection( QObject * _o )
{
	if( _o->property( "demoserver" ).isValid() )
	{
		demoServer * ds = dynamic_cast<demoServer *>(
			_o->property( "demoserver" ).value<QObject *>() );
		if( ds == m_demoServer )
		{
			m_demoServer = NULL;
		}
		delete ds;
		_o->setProperty( "demoserver", QVariant() );
	}
}




void isdServer::processClients( void )
{
	// check all open connections for available data
	QList<QTcpSocket *> sockets = findChildren<QTcpSocket *>();
	for( QList<QTcpSocket *>::iterator it = sockets.begin();
						it != sockets.end(); ++it )
	{
		while( ( *it )->bytesAvailable() > 0 )
		{
			processClient( qtcpsocketDispatcher, *it );
		}
	}
}




void isdServer::checkForPendingActions( void )
{
	QMutexLocker ml( &m_actionMutex );
	while( !m_pendingActions.isEmpty() )
	{
		QString data = m_pendingActions.front().second;
		switch( m_pendingActions.front().first )
		{
			case ISD::StartFullScreenDemo:
			case ISD::StartWindowDemo:
				startDemo( data,
	( m_pendingActions.front().first == ISD::StartFullScreenDemo ) );
				break;

			case ISD::StopDemo:
				stopDemo();
				break;

			case ISD::LockDisplay:
				lockDisplay();
				break;

			case ISD::UnlockDisplay:
				unlockDisplay();
				break;

			case ISD::DisplayTextMessage:
				displayTextMessage( data );
				break;

			case ISD::ViewRemoteDisplay:
			case ISD::RemoteControlDisplay:
				remoteControlDisplay( data,
		( m_pendingActions.front().first == ISD::ViewRemoteDisplay ) );
				break;

			default:
				printf( "unhandled command %d\n",
					(int) m_pendingActions.front().first );
				break;
		}
		m_pendingActions.removeFirst();
	}
}




void isdServer::demoWindowClosed( QObject * )
{
	m_demoClient = NULL;
}




void isdServer::startDemo( const QString & _master_host, bool _fullscreen )
{
	delete m_demoClient;
	m_demoClient = NULL;
/*	if( m_demoServer && _master_host.section( ':', 0, 0 ) ==
				m_demoServer->serverAddress().toString() &&
		_master_host.section( ':', 1, 1 ).toInt() ==
						m_demoServer->serverPort() )
	{
		// somehow we tried to start a demo on demo-server which makes
		// no sense and results in a deadlock...
		return;
	}*/
	// if a demo-server is started, it's likely that the demo was started
	// on master-computer as well therefore we deny starting a demo on
	// hosts on which a demo-server is running
	if( m_demoServer )
	{
		return;
	}

	m_demoClient = new demoClient( _master_host, _fullscreen );
	connect( m_demoClient, SIGNAL( destroyed( QObject * ) ),
				this, SLOT( demoWindowClosed( QObject * ) ) );
}




void isdServer::stopDemo( void )
{
	delete m_demoClient;
	m_demoClient = NULL;
}




void isdServer::lockDisplay( void )
{
	if( m_demoServer )
	{
		return;
	}
	delete m_lockWidget;
	m_lockWidget = new lockWidget();
}




void isdServer::unlockDisplay( void )
{
	delete m_lockWidget;
	m_lockWidget = NULL;
}




void isdServer::displayTextMessage( const QString & _msg )
{
#ifdef SYSTEMTRAY_SUPPORT
	// OS X does not support messages
	if( QSystemTrayIcon::supportsMessages() && __systray_icon )
	{
		__systray_icon->showMessage( tr( "Message from teacher" ),
						_msg,
						QSystemTrayIcon::Information,
									-1 );
	}
	else
	{
#else
	new messageBox( tr( "Message from teacher" ), _msg,
					QPixmap( ":/resources/message.png" ) );
#endif
#ifdef SYSTEMTRAY_SUPPORT
	}
#endif
}




void isdServer::remoteControlDisplay( const QString & _ip, bool _view_only )
{
/*#ifdef BUILD_LINUX
	QProcess::startDetached( QCoreApplication::applicationFilePath() +
					QString( " -rctrl %1 %2" ).
							arg( _ip ).
							arg( _view_only ) );
#else*/
	new remoteControlWidget( _ip, _view_only );
/*#endif*/
}




void isdServer::allowDemoClient( const QString & _host )
{
	foreach( const QHostAddress a,
				QHostInfo::fromName( _host ).addresses() )
	{
		const QString h = a.toString();
		if( !s_allowedDemoClients.contains( h ) )
		{
			s_allowedDemoClients.push_back( h );
		}
	}
}




void isdServer::denyDemoClient( const QString & _host )
{
	foreach( const QHostAddress a,
				QHostInfo::fromName( _host ).addresses() )
	{
		s_allowedDemoClients.removeAll( a.toString() );
	}
}





#ifdef BUILD_LINUX
// helper-class which forwards commands destined to ISD-server. We need this
// when running VNC-server in separate process and VNC-server receives iTALC-
// commands which it can't process
class isdForwarder : public isdConnection
{
public:
	isdForwarder() :
		isdConnection( QHostAddress( QHostAddress::LocalHost ).
						toString() + ":" +
						QString::number( __isd_port ) )
	{
	}

#if 0
	bool handleServerMessage( Q_UINT8 _msg )
	{
		if( _msg != rfbItalcServiceResponse )
		{
			printf( "Unknown message type %d from server. Closing "
				"connection. Will re-open it later.\n", _msg );
			close();
			return( FALSE );
		}

		Q_UINT8 cmd;
		if( !readFromServer( (char *) &cmd, sizeof( cmd ) ) )
		{
			return( FALSE );
		}
		switch( cmd )
		{
			case ISD::UserInformation:
			{
				ISD::msg m( &socketDev() );
				m.receive();
				ISD::msg( &socketDev(), ISD::UserInformation ).
						addArg( "username",
							m.arg( "username" ) ).
						addArg( "homedir",
							m.arg( "homedir" ) ).
									send();
				break;
			}

/*			case ISD::DemoServer_PortInfo:
			{
				ISD::msg m( &socketDev() );
				m.receive();
				ISD::msg( &socketDev(),
						ISD::DemoServer_PortInfo ).
					addArg( "demoserverport",
						m.arg( "demoserverport" ).
							toInt() ).send();
				break;
			}*/

			default:
				printf( "Unknown server response %d\n",
								(int) cmd );
				return( FALSE );
		}

		return( TRUE );
	}


	void handleServerMessages( void )
	{
		printf("handle\n");
		while( hasData() )
		{
			rfbServerToClientMsg msg;
			if( !readFromServer( (char *) &msg,
							sizeof( Q_UINT8 ) ) )
			{
				printf( "Reading message-type failed\n" );
				continue;
			}
			handleServerMessage( msg.type );
		}
	}
#endif

	int processClient( socketDispatcher _sd, void * _user )
	{
		socketDevice sdev( _sd, _user );
		char cmd;
		if( sdev.read( &cmd, sizeof( cmd ) ) == 0 )
		{
			printf( "couldn't read iTALC-request from client...\n" );
			return( FALSE );
		}

		if( cmd == rfbItalcServiceRequest )
		{
			return( processClient( _sd, _user ) );
		}

		// in every case receive message-arguments, even if it's an empty list
		// because this is at leat the int32 with number of items in the list
		ISD::msg msg_in( &sdev, static_cast<ISD::commands>( cmd ) );
		msg_in.receive();

		switch( cmd )
		{
			case ISD::GetUserInformation:
		ISD::msg( &sdev, ISD::UserInformation ).
				addArg( "username",
					localSystem::currentUser() ).
				addArg( "homedir", QDir::homePath() ).send();
				break;

			case ISD::ExecCmds:
				execCmds( msg_in.arg( "cmds" ).toString() );
				break;

			case ISD::StartFullScreenDemo:
			case ISD::StartWindowDemo:
				startDemo( msg_in.arg( "ip" ).toString(),
					cmd == ISD::StartFullScreenDemo );
				break;

			case ISD::ViewRemoteDisplay:
			case ISD::RemoteControlDisplay:
				remoteControlDisplay( msg_in.arg( "ip" ).
								toString(),
						cmd == ISD::ViewRemoteDisplay );
				break;

			case ISD::DisplayTextMessage:
				displayTextMessage( msg_in.arg( "msg" ).
								toString() );
				break;

			case ISD::LockDisplay:
				lockDisplay();
				break;

			case ISD::UnlockDisplay:
				unlockDisplay();
				break;

			case ISD::StopDemo:
				stopDemo();
				break;

			case ISD::LogonUserCmd:
				logonUser( msg_in.arg( "uname" ).toString(),
					msg_in.arg( "passwd" ).toString(),
					msg_in.arg( "domain" ).toString() );
				break;

			case ISD::LogoutUser:
				logoutUser();
				break;

			case ISD::WakeOtherComputer:
				wakeOtherComputer( 
					msg_in.arg( "mac" ).toString() );
				break;

			case ISD::PowerDownComputer:
				powerDownComputer();
				break;

			case ISD::RestartComputer:
				restartComputer();
				break;

			case ISD::DemoServer_Run:
				demoServerRun( msg_in.arg( "quality" ).toInt(),
						msg_in.arg( "port" ).toInt() );
				break;

			case ISD::DemoServer_Stop:
				demoServerStop();
				break;

			case ISD::DemoServer_AllowClient:
				demoServerAllowClient(
					msg_in.arg( "client" ).toString() );
				break;

			case ISD::DemoServer_DenyClient:
				demoServerDenyClient(
					msg_in.arg( "client" ).toString() );
				break;

			default:
				printf( "!!! cmd %d not implemented!\n", cmd );
				break;
		}

		return( TRUE );
	}


protected:
	virtual states authAgainstServer( const italcAuthTypes _try_auth_type )
	{
		return( isdConnection::authAgainstServer(
					ItalcAuthChallengeViaAuthFile ) );
	}

} ;

static isdForwarder * __isd_forwarder = NULL;

#endif


int processItalcClient( socketDispatcher _sd, void * _user )
{
	if( __isd_server )
	{
		return( __isd_server->processClient( _sd, _user ) );
	}

#ifdef BUILD_LINUX
	if( !__isd_forwarder )
	{
		__isd_forwarder = new isdForwarder();
	}

	if( __isd_forwarder->state() != isdForwarder::Connected )
	{
		__isd_forwarder->open();
	}

	//__isd_forwarder->handleServerMessages();

	return( __isd_forwarder->processClient( _sd, _user ) );
#endif
	return( 0 );
}




#include "isd_server.moc"

