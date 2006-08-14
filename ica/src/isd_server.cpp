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


#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
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


static isdServer * __isd_server = NULL;

QByteArray isdServer::s_appInternalChallenge;




isdServer::isdServer( const quint16 _isd_port, const quint16 _ivs_port,
						int _argc, char * * _argv ) :
	QTcpServer(),
#warning: change this
	m_authType( ItalcAuthNone ),
	m_ivs( new IVS( _ivs_port, _argc, _argv ) ),
	m_demoClient( NULL ),
	m_demoServer( NULL ),
	m_lockWidget( NULL )
{
	if( __isd_server ||
			listen( QHostAddress::LocalHost, _isd_port ) == FALSE )
	{
		// uh oh, already an ISD running or port isn't available...
		printf( "Could not start ISD server: %s\n",
					errorString().toAscii().constData() );
	}

	m_ivs->start( QThread::HighPriority );

	connect( this, SIGNAL( newConnection() ),
			this, SLOT( acceptNewConnection() ) );

	QTimer * t = new QTimer( this );
	connect( t, SIGNAL( timeout() ), this,
					SLOT( checkForPendingActions() ) );
	// as things like creating a demo-window, remote-control-view etc. can
	// only be done by GUI-thread we push all actions into a list and
	// process this list later in a slot called by the GUI-thread every 500s
	t->start( 500 );

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




bool isdServer::authSecTypeItalc( socketDispatcher _sd, void * _user,
						italcAuthTypes _auth_type )
{
	socketDevice sdev( _sd, _user );
#warning: TODO: remove this one day!
	_auth_type = ItalcAuthNone;
	sdev.write( QVariant( (int) _auth_type ) );

	italcAuthResults result = ItalcAuthFailed;
	switch( _auth_type )
	{
		// no authentication
		case ItalcAuthNone:
			result = ItalcAuthOK;
			break;

		// host has to be in list of allowed hosts
		case ItalcAuthHostBased:
		{
			// find out IP of host
			const int MAX_HOST_LEN = 255;
			char host[MAX_HOST_LEN];
			_sd( host, MAX_HOST_LEN, SocketGetIPBoundTo, NULL );
			host[MAX_HOST_LEN] = 0;

			// create a list of all known addresses of host
			QList<QHostAddress> addr =
					QHostInfo::fromName( host ).addresses();
			if( !addr.isEmpty() )
			{
				// check each address for existence in allowed-
				// client-list
				foreach( const QHostAddress a, addr )
				{
					if( m_allowedDemoClients.contains(
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
		case ItalcAuthDSA:
		{
			// generate data to sign and send to client
			const QByteArray chall = dsaKey::generateChallenge();
			sdev.write( QVariant( chall ) );

			// get user-role
			const ISD::userRoles urole =
				static_cast<ISD::userRoles>(
					sdev.read().toString().toInt() );

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
			// is our client able to read this byte-array? if so,
			// it's for sure running inside the same app
			result = ( sdev.read().toByteArray() ==
						s_appInternalChallenge ) ?
						ItalcAuthOK : ItalcAuthFailed;
			break;
		}
	}

	sdev.write( QVariant( (int) result ) );

	return( result == ItalcAuthOK );
}




int isdServer::processClient( socketDispatcher _sd, void * _user )
{
	socketDevice sdev( _sd, _user );
	char cmd;
	if( sdev.read( &cmd, sizeof( cmd ) ) == 0 )
	{
		printf( "couldn't read iTALC-request from client...\n" );
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

		case ISD::ForceUserLogout:
			localSystem::logoutUser();
			break;

		case ISD::WakeOtherComputer:
			localSystem::sendWakeOnLANPacket( 
					msg_in.arg( "mac" ).toString() );
			break;

		case ISD::PowerDownComputer:
			localSystem::powerDown();
			break;

		case ISD::RestartComputer:
			localSystem::reboot();
			break;

/*		case ISD::IVS_Run:
			m_ivs->start( QThread::HighestPriority );
			break;

		case ISD::IVS_Terminate:
			m_ivs->terminate();
			break;*/

		case ISD::DemoServer_Run:
			// make sure any running demo-server is destroyed
			delete m_demoServer;
			// start demo-server on local IVS
			m_demoServer = new demoServer( m_ivs->serverPort() );
			if( m_demoServer->serverPort() )
			{
				// tell iTALC-master at which port the demo-
				// server is running
				ISD::msg( &sdev, ISD::DemoServer_PortInfo ).
					addArg( "demoserverport",
						m_demoServer->serverPort() ).
									send();
			}
			break;

		case ISD::DemoServer_Stop:
			delete m_demoServer;
			m_demoServer = NULL;
			break;

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

	if( !::authSecTypeItalc( _sd.sockDispatcher(), _sd.user(),
								_auth_type ) )
	{
		return( FALSE );
	}

	return( TRUE );
}




void isdServer::acceptNewConnection( void )
{
	QTcpSocket * sock = nextPendingConnection();
	socketDevice sd( qtcpsocketDispatcher, sock );

	if( !protocolInitialization( sd, m_authType ) )
	{
		delete sock;
		return;
	}

	// now we're ready to start the normal interaction with the client,
	// so make sure, we get informed about new requests
	connect( sock, SIGNAL( disconnected() ), sock, SLOT( deleteLater() ) );
	connect( sock, SIGNAL( readyRead() ), this, SLOT( processClients() ) );
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
	if( m_demoServer && _master_host.section( ':', 0, 0 ) ==
				m_demoServer->serverAddress().toString() &&
		_master_host.section( ':', 1, 1 ).toInt() ==
						m_demoServer->serverPort() )
	{
		// somehow we tried to start a demo on demo-server which makes
		// no sense and results in a deadlock...
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
	new messageBox( tr( "Message from teacher" ), _msg,
					QPixmap( ":/resources/message.png" ) );
}




void isdServer::remoteControlDisplay( const QString & _ip, bool _view_only )
{
	new remoteControlWidget( _ip, _view_only );
}




void isdServer::allowDemoClient( const QString & _host )
{
	foreach( const QHostAddress a,
				QHostInfo::fromName( _host ).addresses() )
	{
		const QString h = a.toString();
		if( !m_allowedDemoClients.contains( h ) )
		{
			printf("allow: %s\n", h.toAscii().constData());
			m_allowedDemoClients.push_back( h );
		}
	}
}




void isdServer::denyDemoClient( const QString & _host )
{
	foreach( const QHostAddress a,
				QHostInfo::fromName( _host ).addresses() )
	{
		m_allowedDemoClients.removeAll( a.toString() );
	}
}







int processItalcClient( socketDispatcher _sd, void * _user )
{
	return( __isd_server->processClient( _sd, _user ) );
}




bool authSecTypeItalc( socketDispatcher _sd, void * _user, italcAuthTypes _at )
{
	return( __isd_server->authSecTypeItalc( _sd, _user, _at ) );
}



#include "isd_server.moc"

