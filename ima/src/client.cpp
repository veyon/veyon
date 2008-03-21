/*
 * client.cpp - code for client-windows, which are displayed in several
 *              instances in the main-window of iTALC
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

 
#include <QtGui/QCloseEvent>
#include <QtGui/QLinearGradient>
#include <QtGui/QMenu>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QScrollArea>


#include "main_window.h"
#include "client.h"
#include "ivs_connection.h"
#include "classroom_manager.h"
#include "cmd_input_dialog.h"
#include "local_system.h"
#include "snapshot_list.h"
#include "dialogs.h"
#include "messagebox.h"
#include "remote_control_widget.h"



const QSize DEFAULT_CLIENT_SIZE( 256, 192 );
const int DECO_WIDTH = 4;
const int TITLE_HEIGHT = 20;
const QPoint CONTENT_OFFSET( DECO_WIDTH, DECO_WIDTH + TITLE_HEIGHT ); 
const QSize CONTENT_SIZE_SUB( 2*DECO_WIDTH, 2*DECO_WIDTH + TITLE_HEIGHT ); 


const client::clientCommand client::s_commands[client::Cmd_CmdCount] =
{

	{ Cmd_None,		NULL,				(char *) "",								(char *) "",			FALSE	},
	{ Cmd_Reload,		&client::reload,		(char *) "",								(char *) "", 			TRUE	},
	{ Cmd_ViewLive,		&client::viewLive,		(char *) QT_TRANSLATE_NOOP( "client", "View live" ),			(char *) "viewmag.png",		FALSE	},
	{ Cmd_RemoteControl,	&client::remoteControl,		(char *) QT_TRANSLATE_NOOP( "client", "Remote control" ),		(char *) "remote_control.png",	FALSE	},
	{ Cmd_ClientDemo,	&client::clientDemo,		(char *) QT_TRANSLATE_NOOP( "client", "Let student show demo" ),		(char *) "client_demo.png",	FALSE	},
	{ Cmd_SendTextMessage,	&client::sendTextMessage,	(char *) QT_TRANSLATE_NOOP( "client", "Send text message" ),		(char *) "text_message.png",	TRUE	},
	{ Cmd_LogonUser,	&client::logonUser,		(char *) QT_TRANSLATE_NOOP( "client", "Logon user" ),			(char *) "remotelogon.png",	FALSE	},
	{ Cmd_LogoutUser,	&client::logoutUser,		(char *) QT_TRANSLATE_NOOP( "client", "Logout user" ),			(char *) "logout.png",		TRUE	},
	{ Cmd_Snapshot,		&client::snapshot,		(char *) QT_TRANSLATE_NOOP( "client", "Take a snapshot" ),		(char *) "snapshot.png", 	TRUE	},
	{ Cmd_PowerOn,		&client::powerOn,		(char *) QT_TRANSLATE_NOOP( "client", "Power on" ),			(char *) "power_on.png",		FALSE	},
	{ Cmd_Reboot,		&client::reboot,		(char *) QT_TRANSLATE_NOOP( "client", "Reboot" ),			(char *) "reboot.png",		FALSE	},
	{ Cmd_PowerDown,	&client::powerDown,		(char *) QT_TRANSLATE_NOOP( "client", "Power down" ),			(char *) "power_off.png",	FALSE	},
	{ Cmd_ExecCmds,		&client::execCmds,		(char *) QT_TRANSLATE_NOOP( "client", "Execute commands" ),		(char *) "run.png", 		FALSE	}

} ;


// resolve static symbols...
QHash<int, client *> client::s_clientIDs;

bool client::s_reloadSnapshotList = FALSE;


class closeButton : public QWidget
{
public:
	closeButton( QWidget * _parent ) :
		QWidget( _parent ),
		m_mouseOver( FALSE )
	{
		setFixedSize( 18, 18 );
	}
	virtual ~closeButton()
	{
	}

	virtual void enterEvent( QEvent * _fe )
	{
		m_mouseOver = TRUE;
		QWidget::enterEvent( _fe );
		update();
	}
	virtual void leaveEvent( QEvent * _fe )
	{
		m_mouseOver = FALSE;
		QWidget::leaveEvent( _fe );
		update();
	}
	virtual void paintEvent( QPaintEvent * _pe )
	{
		QPainter p( this );
		p.setRenderHint( QPainter::Antialiasing, TRUE );
		p.setPen( QColor( 128, 128, 128 ) );
		p.setBrush( m_mouseOver ? Qt::gray : Qt::white );
		p.drawRoundRect( QRectF( 0.5, 0.5, 16, 16 ), 20, 20 );
		QPen pen( QColor( 64, 64, 64 ) );

		pen.setWidth( 3 );
		p.setPen( pen );
		p.drawLine( QLineF( 4.5, 4.5,  12.5, 12.5 ) );
		p.drawLine( QLineF( 4.5, 12.5, 12.5, 4.5 ) );
	}

	virtual void mousePressEvent( QMouseEvent * )
	{
	}

	virtual void mouseReleaseEvent( QMouseEvent * _me )
	{
		if( rect().contains( _me->pos() ) )
		{
			parentWidget()->close();
		}
	}

private:
	bool m_mouseOver;

} ;





client::client( const QString & _local_ip,
		const QString & _mac, const QString & _name,
		types _type, classRoom * _class_room,
					mainWindow * _main_window, int _id ) :
	QWidget( _main_window->workspace() ),
	m_mainWindow( _main_window ),
	m_connection( NULL ),
	m_clickPoint( -1, -1 ),
	m_origPos( -1, -1 ),
	m_name( _name ),
	m_localIP( _local_ip ),
	m_mac( _mac ),
	m_type( _type ),
	m_reloadsAfterReset( 0 ),
	m_mode( Mode_Overview ),
	m_user( "" ),
	m_makeSnapshot( FALSE ),
	m_state( State_Unkown ),
	m_syncMutex(),
	m_classRoomItem( NULL ),
	m_contextMenu( NULL ),
	m_updateThread( NULL )
{
	new closeButton( this );

	if( _id <= 0 || clientFromID( _id ) != NULL )
	{
		_id = freeID();
	}
	s_clientIDs[_id] = this;


	m_classRoomItem = new classRoomItem( this, _class_room );

	m_connection = new ivsConnection( m_localIP,
						ivsConnection::QualityLow );

	setAttribute( Qt::WA_OpaquePaintEvent );
	setWindowIcon( QPixmap( ":/resources/classroom_manager.png" ) );

/*	setWhatsThis( tr( "This is a client-window. It either displays the "
				"screen of the according client or a message "
				"about the state of this client (no user "
				"logged in/powered off) is shown. You can "
				"click with the right mouse-button and an "
				"action-menu for this client will appear. "
				"You can also close this client-window. "
				"To open it again, open the classroom-manager-"
				"workspace and search this client and double-"
				"click it.\nYou can change the size of this "
				"(and all other visible) client-windows by "
				"using the functions for increasing, "
				"decreasing or optimizing the client-window-"
								"size." ) );*/

	setFixedSize( DEFAULT_CLIENT_SIZE );
	//resize( DEFAULT_CLIENT_SIZE );

	QMenu * tb = new QMenu( this );
	m_contextMenu = tb;
	connect( tb, SIGNAL( triggered( QAction * ) ), this,
					SLOT( processCmdSlot( QAction * ) ) );
	tb->addAction( scaled( ":/resources/overview_mode.png", 16, 16 ),
			tr( "Overview" ) )->
					setData( Cmd_CmdCount + Mode_Overview );
	tb->addAction( scaled( ":/resources/fullscreen_demo.png", 16, 16 ),
						tr( "Fullscreen demo" ) )->
				setData( Cmd_CmdCount + Mode_FullscreenDemo );
	tb->addAction( scaled( ":/resources/window_demo.png", 16, 16 ),
						tr( "Window demo" ) )->
				setData( Cmd_CmdCount + Mode_WindowDemo );
	tb->addAction( scaled( ":/resources/locked.png", 16, 16 ),
						tr( "Locked display" ) )->
				setData( Cmd_CmdCount + Mode_Locked );
	tb->addSeparator();
	for( int i = Cmd_ViewLive; i < Cmd_CmdCount; ++i )
	{
		QAction * a = tb->addAction( scaled(
			QString( ":/resources/" ) + s_commands[i].m_icon,
								16, 16 ),
						tr( s_commands[i].m_name ) );
		a->setData( i );
		if( s_commands[i].m_insertSep )
		{
			tb->addSeparator();
		}
	}

	m_updateThread = new updateThread( this );
}




client::~client()
{
	changeMode( client::Mode_Overview, m_mainWindow->localISD() );
	m_updateThread->quit();
	m_updateThread->wait();
	m_updateThread->update();
	delete m_updateThread;

	m_syncMutex.lock();
	delete m_connection;
	m_connection = NULL;
	m_syncMutex.unlock();

	delete m_classRoomItem;
}




int client::id( void ) const
{
	QHash<int, client *>::const_iterator it;
	for( it = s_clientIDs.begin(); it != s_clientIDs.end(); ++it )
	{
		if( it.value() == this )
		{
			return( it.key() );
		}
	}
	return( 0 );
}




client * client::clientFromID( int _id )
{
	if( s_clientIDs.contains( _id ) )
	{
		return( s_clientIDs[_id] );
	}
	return( NULL );
}




int client::freeID( void )
{
	const int ID_MAX = 1 << 20;

	int id;
	while( s_clientIDs.contains( id = static_cast<int>( (float) rand() /
						RAND_MAX * ID_MAX + 1 ) ) )
	{
	}
	return( id );
}




void client::changeMode( const modes _new_mode, isdConnection * _conn )
{
	if( _new_mode != m_mode )
	{
		//m_syncMutex.lock();
		switch( m_mode )
		{
			case Mode_Overview:
			case Mode_Unknown:
				break;
			case Mode_FullscreenDemo:
			case Mode_WindowDemo:
				_conn->demoServerDenyClient( m_localIP );
				m_updateThread->enqueueCommand(
						updateThread::Cmd_StopDemo );
				break;
			case Mode_Locked:
				m_updateThread->enqueueCommand(
						updateThread::Cmd_UnlockScreen );
				break;
		}
		switch( m_mode = _new_mode )
		{
			case Mode_Overview:
			case Mode_Unknown:
				break;
			case Mode_FullscreenDemo:
			case Mode_WindowDemo:
				_conn->demoServerAllowClient( m_localIP );
				m_updateThread->enqueueCommand(
						updateThread::Cmd_StartDemo,
	QList<QVariant>()
			<< _conn->host() + ":" + QString::number(
						_conn->demoServerPort() )
			<< (int)( m_mode == Mode_FullscreenDemo ) );
				//m_mode = Mode_FullscreenDemo;
				break;
			case Mode_Locked:
				m_updateThread->enqueueCommand(
						updateThread::Cmd_LockScreen );
				break;
		}
		//m_syncMutex.unlock();
	}
	// if connection was lost while sending commands such as stop-demo,
	// there should be a way for switching back into normal mode, that's
	// why we offer this lines
	else if( m_mode == Mode_Overview )
	{
		if( _conn != NULL )
		{
			_conn->demoServerDenyClient( m_localIP );
		}
		m_updateThread->enqueueCommand( updateThread::Cmd_StopDemo );
		m_updateThread->enqueueCommand( updateThread::Cmd_UnlockScreen );
	}
}




void client::setClassRoom( classRoom * _cr )
{
	delete m_classRoomItem;

	m_classRoomItem = new classRoomItem( this, _cr );
}




void client::resetConnection( void )
{
	m_updateThread->enqueueCommand( updateThread::Cmd_ResetConnection,
								m_localIP );
}




void client::update( void )
{
	m_state = currentState();
	QWidget::update();
}




void client::createActionMenu( QMenu * _m )
{
	connect( _m, SIGNAL( triggered( QAction * ) ), this,
					SLOT( processCmdSlot( QAction * ) ) );
	_m->addActions( m_contextMenu->actions() );
}




void client::processCmd( clientCmds _cmd, const QString & _u_data )
{
	if( _cmd < 0 || _cmd >= Cmd_CmdCount )
	{
		return;
	}

	( this->*( client::s_commands[_cmd].m_exec ) )( _u_data );
}




void client::processCmdSlot( QAction * _action )
{
	int a = _action->data().toInt();
	if( a >= Cmd_ViewLive && a < Cmd_CmdCount )
	{
		processCmd( static_cast<clientCmds>( a ) );
	}
	else if( a >= Cmd_CmdCount+Mode_Overview &&
						a < Cmd_CmdCount+Mode_Unknown )
	{
		changeMode( static_cast<modes>( a - Cmd_CmdCount ),
						m_mainWindow->localISD() );
	}
}




bool client::userLoggedIn( void )
{
	QMutexLocker ml( &m_syncMutex );

	return( m_connection->state() == ivsConnection::Connected ||
		m_connection->reset( m_localIP, &m_reloadsAfterReset ) ==
						ivsConnection::Connected );
}




void client::closeEvent( QCloseEvent * _ce )
{
	// make sure, client isn't forgotten by teacher after being hidden
	changeMode( Mode_Overview, m_mainWindow->localISD() );
	hide();
	_ce->ignore();
}




void client::contextMenuEvent( QContextMenuEvent * )
{
	m_contextMenu->exec( QCursor::pos() );
}




void client::hideEvent( QHideEvent * )
{
	if( isMinimized() )
	{
		hide();
	}

	if( m_classRoomItem != NULL )
	{
		m_classRoomItem->setVisible( FALSE );
	}
}




void client::enlarge()
{
	if ( m_origSize.isValid() ) 
	{
		QWidget * workspace = m_mainWindow->workspace();
		QSize s = QSize( m_origSize );
		
		s.scale( workspace->parentWidget()->size(), Qt::KeepAspectRatio );
		setFixedSize( s );

		/* centralize */
		QSize offset = ( workspace->parentWidget()->size() - s ) / 2;
		move( offset.width() - workspace->x(),
			offset.height() - workspace->y() );
	}
}




void client::zoom()
{
	m_origPos = pos();
	m_origSize = size();
	/* Delay zooming before we are sure that
	 * this is not just a fast click.
	 */
	QTimer::singleShot( 300, this,
			SLOT( enlarge() ) );
}




void client::zoomBack()
{
	if ( m_origSize.isValid() )
	{	
		move( m_origPos );
		setFixedSize( m_origSize );
		/* reset value: */
		m_origSize = QSize();
	}
}




void client::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		raise();
		m_clickPoint = _me->globalPos();
		m_origPos = pos();
		zoom();
	}
	else
	{
		QWidget::mousePressEvent( _me );
	}
}




void client::mouseMoveEvent( QMouseEvent * _me )
{
	if( m_clickPoint.x() >= 0 )
	{
		zoomBack();
		move( m_origPos + _me->globalPos() - m_clickPoint );
		parentWidget()->updateGeometry();
	}
	else
	{
		QWidget::mouseMoveEvent( _me );
	}
}




void client::mouseReleaseEvent( QMouseEvent * _me )
{
	zoomBack();
	m_clickPoint = QPoint( -1, -1 );
	QWidget::mouseReleaseEvent( _me );
}




void client::mouseDoubleClickEvent( QMouseEvent * _me )
{
	if( m_mainWindow->getClassroomManager()->clientDblClickAction() == 0 )
	{
		remoteControl( "" );
	}
	else
	{
		viewLive( "" );
	}
}




void client::paintEvent( QPaintEvent * _pe )
{
	static QImage * img_unknown = NULL;
	static QImage * img_no_user = NULL;
	static QImage * img_host_unreachable = NULL;
	static QImage * img_demo = NULL;
	static QImage * img_locked = NULL;

	if( img_unknown == NULL )
		img_unknown = new QImage( ":/resources/error.png" );
	if( img_no_user == NULL )
		img_no_user = new QImage( ":/resources/no_user.png" );
	if( img_host_unreachable == NULL )
		img_host_unreachable = new QImage( ":/resources/host_unreachable.png" );
	if( img_demo == NULL )
		img_demo = new QImage( ":/resources/window_demo.png" );
	if( img_locked == NULL )
		img_locked = new QImage( ":/resources/locked.png" );

	QPainter p( this );
	p.setBrush( Qt::white );
	p.setPen( Qt::black );
	p.drawRect( QRect( 0, 0, width()-1, height()-1 ) );
	p.setRenderHint( QPainter::Antialiasing, TRUE );

	bool showUsername = m_mainWindow->getClassroomManager()->showUsername();
	const QString s = (showUsername && m_user != "") ? m_user :
		( m_name + " (" + m_classRoomItem->parent()->text( 0 ) +
									")" );
	QFont f = p.font();
	f.setBold( TRUE );
	p.setFont( f );
	p.setPen( Qt::gray );
	p.drawText( 11, TITLE_HEIGHT-3, s );
	p.setPen( Qt::black );
	p.drawText( 10, TITLE_HEIGHT-4, s );

	if( m_connection->state() == ivsConnection::Connected &&
						m_mode == Mode_Overview )
	{
		p.drawImage( CONTENT_OFFSET, m_connection->scaledScreen() );
	}
	else
	{
	/*	p.drawImage( _pe->rect().topLeft() + CONTENT_OFFSET,
						m_statePixmap, _pe->rect() );*/
	const int aw = width() - 2*DECO_WIDTH;
	const int ah = height() - CONTENT_SIZE_SUB.height() - DECO_WIDTH;

	QImage * pm = img_unknown;
	QString msg = tr( "Unknown state" );

	switch( m_state )
	{
		case State_Overview:
			return;
		case State_NoUserLoggedIn:
			pm = img_no_user;
			msg = tr( "No user logged in" );
			break;
		case State_Unreachable:
			pm = img_host_unreachable;
			msg = tr( "Host unreachable" );
			break;
		case State_Demo:
			pm = img_demo;
			msg = tr( "Demo running" );
			break;
		case State_Locked:
			pm = img_locked;
			msg = tr( "Desktop locked" );
			break;
		default:
			break;
	}

	QFont f = p.font();
	f.setBold( TRUE );
	f.setPointSize( f.pointSize() + 1 );
	p.setFont( f );

	QRect r = p.boundingRect( QRect( 5, 0, aw-10, 10 ),
				Qt::TextWordWrap | Qt::AlignCenter, msg );
	QSize s( pm->size() );
	s.scale( aw-10, ah-r.height()-20, Qt::KeepAspectRatio );
	p.drawImage( ( aw-s.width() ) / 2, height()-ah,
						fastQImage( *pm ).scaled( s ) );

	p.setPen( QColor( 0, 0, 0 ) );
	p.drawText( QRect( 5, height()-r.height()-10, aw - 10,
						r.height() ),
				Qt::TextWordWrap | Qt::AlignCenter, msg );
	}

	if( m_makeSnapshot )
	{
		QMutexLocker ml( &m_syncMutex );
		m_makeSnapshot = FALSE;
		if( m_connection->takeSnapshot() )
		{
			s_reloadSnapshotList = TRUE;
		}
	}

}




void client::resizeEvent( QResizeEvent * _re )
{
	findChild<closeButton*>()->move( width()-21, 4 );
	m_connection->setScaledSize( size() - CONTENT_SIZE_SUB );
	m_connection->rescaleScreen();
	QWidget::resizeEvent( _re );
}





void client::showEvent( QShowEvent * )
{
	if( m_classRoomItem != NULL )
	{
		m_classRoomItem->setVisible( TRUE );
	}
}




void client::reload( const QString & _update )
{
	QString tooltip = "";
	if( userLoggedIn() )
	{
		m_syncMutex.lock();

		// for some reason we must not send user-information-requests
		// while first messages being exchanged between client and
		// server
		if( m_reloadsAfterReset > 5 )
		{
			m_connection->sendGetUserInformationRequest();
		}
		else
		{
			++m_reloadsAfterReset;
		}
		//if( m_connection->sendGetUserInformationRequest() )
		{
			// only send a framebuffer-update-request if client
			// is in (over)view-mode
			m_connection->handleServerMessages(
						m_mode == Mode_Overview );
		}

		m_user = m_connection->user();
		// at least set tooltip with user-name if it is not displayed
		// in title-bar
		if( !m_mainWindow->getClassroomManager()->showUsername() )
		{
			tooltip = m_user;
		}
		m_syncMutex.unlock();
	}
	else
	{
		m_user = "";
	}

	if( toolTip() != tooltip )
	{
		setToolTip( tooltip );
	}

	// if we are called out of main-thread, we may update...
	if( _update == CONFIRM_YES )
	{
		update();
	}

	if( m_classRoomItem )
	{
		m_classRoomItem->setUser( m_user );
	}
}




void client::clientDemo( const QString & )
{
	classroomManager * cm = m_mainWindow->getClassroomManager();
	cm->changeGlobalClientMode( Mode_Overview );

	isdConnection * conn = m_mainWindow->localISD();
	QVector<client *> vc = cm->visibleClients();

	foreach( client * cl, vc )
	{
		if( cl != this )
		{
			cl->changeMode( Mode_FullscreenDemo, conn );
		}
	}

	//m_mainWindow->checkModeButton( client::Mode_FullscreenDemo );

	m_mainWindow->remoteControlDisplay( m_localIP, TRUE, TRUE );
}




void client::viewLive( const QString & )
{
	changeMode( Mode_Overview, m_mainWindow->localISD() );

	m_mainWindow->remoteControlDisplay( m_localIP, TRUE );
}




void client::remoteControl( const QString & )
{
	changeMode( Mode_Overview, m_mainWindow->localISD() );

	m_mainWindow->remoteControlDisplay( m_localIP );
}




void client::sendTextMessage( const QString & _msg )
{
	if( _msg.isEmpty() )
	{
		QString msg;

		textMessageDialog tmd( msg, this );
		if( tmd.exec() == QDialog::Accepted && msg != "" )
		{
			sendTextMessage( msg );
		}
	}
	else
	{
		m_updateThread->enqueueCommand(
				updateThread::Cmd_SendTextMessage, _msg );
	}
}




void client::logonUser( const QString & _uname_and_pw )
{

	if( _uname_and_pw.isEmpty() )
	{
		remoteLogonDialog mld( this );
		if( mld.exec() == QDialog::Accepted &&
			!mld.userName().isEmpty() && !mld.password().isEmpty() )
		{
			logonUser( mld.userName() + "*" + mld.password() +
							"*" + mld.domain() );
		}
	}
	else
	{
		m_updateThread->enqueueCommand( updateThread::Cmd_LogonUser,
								_uname_and_pw );
	}
}




void client::logoutUser( const QString & _confirm )
{
	m_updateThread->enqueueCommand( updateThread::Cmd_LogoutUser );
}




void client::snapshot( const QString & )
{
	m_makeSnapshot = TRUE;
}





void client::powerOn( const QString & )
{
	m_mainWindow->localISD()->wakeOtherComputer( m_mac );
}




void client::reboot( const QString & _confirm )
{
	m_updateThread->enqueueCommand( updateThread::Cmd_Reboot );
}





void client::powerDown( const QString & _confirm )
{
	m_updateThread->enqueueCommand( updateThread::Cmd_PowerDown );
}




void client::execCmds( const QString & _cmds )
{
	if( _cmds.isEmpty() )
	{
		QString cmds;

		cmdInputDialog cmd_input_dialog( cmds, this );
		if( cmd_input_dialog.exec() == QDialog::Accepted &&
							!cmds.isEmpty() )
		{
			execCmds( cmds );
		}
	}
	else
	{
		m_updateThread->enqueueCommand( updateThread::Cmd_ExecCmds,
									_cmds );
	}
}



client::states client::currentState( void ) const
{
	//QMutexLocker m( &m_syncMutex );
	switch( m_mode )
	{
		case Mode_Overview:
			if( m_connection->state() == ivsConnection::Connected )
			{
				return( State_Overview );
			}
			else if( m_connection->state() ==
					ivsConnection::ConnectionRefused )
			{
				return( State_NoUserLoggedIn );
			}
			return( State_Unreachable );

		case Mode_FullscreenDemo:
		case Mode_WindowDemo:
			return( State_Demo );

		case Mode_Locked:
			return( State_Locked );

		default:
			break;
	}

	return( State_Unkown );
}












updateThread::updateThread( client * _client ) :
	QThread(),
	m_client( _client )
{
	start( LowPriority );
}




void updateThread::update( void )
{
	m_queueMutex.lock();
	m_client->m_syncMutex.lock();
	while( !m_queue.isEmpty() )
	{
		const queueItem i = m_queue.dequeue();
		m_queueMutex.unlock();
		switch( i.first )
		{
			case Cmd_ResetConnection:
				m_client->m_connection->reset(
						i.second.toString() );
				break;
			case Cmd_StartDemo:
				m_client->m_connection->startDemo(
					i.second.toList()[0].toString(),
					i.second.toList()[1].toInt() );
				break;
			case Cmd_StopDemo:
				m_client->m_connection->stopDemo();
				break;
			case Cmd_LockScreen:
				m_client->m_connection->lockDisplay();
				break;
			case Cmd_UnlockScreen:
				m_client->m_connection->unlockDisplay();
				break;
			case Cmd_SendTextMessage:
				m_client->m_connection->
					displayTextMessage(
						i.second.toString() );
				break;
			case Cmd_LogonUser:
			{
				const QString s = i.second.toString();
				const int pos = s.indexOf( '*' );
				const int pos2 = s.lastIndexOf( '*' );
				m_client->m_connection->logonUser(
						s.left( pos ),
						s.mid( pos + 1,
							pos2-pos-1 ),
						s.mid( pos2 + 1 ) );
				break;
			}
			case Cmd_LogoutUser:
				m_client->m_connection->logoutUser();
				break;
			case Cmd_Reboot:
				m_client->m_connection->
						restartComputer();
				break;
			case Cmd_PowerDown:
				m_client->m_connection->
						powerDownComputer();
				break;
			case Cmd_ExecCmds:
				m_client->m_connection->execCmds(
						i.second.toString() );
				break;
		}
		m_queueMutex.lock();
	}
	m_client->m_syncMutex.unlock();
	m_queueMutex.unlock();

	if( m_client->isVisible() )
	{
		if( !m_client->m_mainWindow->remoteControlRunning() &&
						!mainWindow::atExit() )
		{
			m_client->processCmd( client::Cmd_Reload, CONFIRM_NO );
		}
	}
	else if( m_client->m_connection->state() == ivsConnection::Connected )
	{
		m_client->m_connection->close();
	}
}




void updateThread::run( void )
{
	QTimer t;
	connect( &t, SIGNAL( timeout() ), this, SLOT( update() ),
							Qt::DirectConnection );
	t.start( m_client->m_mainWindow->getClassroomManager()->updateInterval()
								* 1000 );
	exec();
	update();
	m_client->m_connection->close();
}




#include "client.moc"

