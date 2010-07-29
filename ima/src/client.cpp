/*
 * client.cpp - code for client-windows, which are displayed in several
 *              instances in the main-window of iTALC
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include <QtGui/QMessageBox>


#include "MainWindow.h"
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
const int TITLE_HEIGHT = 23;
const QPoint CONTENT_OFFSET( DECO_WIDTH, DECO_WIDTH + TITLE_HEIGHT ); 
const QSize CONTENT_SIZE_SUB( 2*DECO_WIDTH, 2*DECO_WIDTH + TITLE_HEIGHT ); 


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




clientAction::clientAction( type _type, QObject * _parent, int _flags ) : 
	QAction( _parent ),
	m_type( _type ),
	m_flags( _flags )
{
}




clientAction::clientAction( type _type, const QIcon & _icon, const QString & _text,
				QObject * _parent, int _flags ) : 
	QAction( _icon, _text, _parent ),
	m_type( _type ),
	m_flags( _flags )
{
}




void clientAction::process( QVector<client *> _clients, targetGroup _target )
{
	switch ( m_type )
	{
		case Overview:
			foreach( client * cl, _clients )
			{
				cl->changeMode( client::Mode_Overview );
			}
			break;
			
		case FullscreenDemo:
			foreach( client * cl, _clients )
			{
				cl->changeMode( client::Mode_FullscreenDemo );
			}
			break;
			
		case WindowDemo:
			foreach( client * cl, _clients )
			{
				cl->changeMode( client::Mode_WindowDemo );
			}
			break;
			
		case Locked:
			foreach( client * cl, _clients )
			{
				cl->changeMode( client::Mode_Locked );
			}
			break;
			
		case ViewLive:
			foreach( client * cl, _clients )
			{
				cl->viewLive();
			}
			break;
			
		case RemoteControl:
			foreach( client * cl, _clients )
			{
				cl->remoteControl();
			}
			break;
			
		case ClientDemo:
			foreach( client * cl, _clients )
			{
				cl->clientDemo();
			}
			break;
			
		case SendTextMessage:
			{
				QString msg;
				textMessageDialog tmd( msg );

				if( tmd.exec() == QDialog::Accepted &&
					!msg.isEmpty() )
				{ 
					foreach( client * cl, _clients )
					{
						cl->sendTextMessage( msg );
					}
				}
			}

			break;

		case LogonUser:
			{
				remoteLogonDialog mld;

				if( mld.exec() == QDialog::Accepted &&
					!mld.userName().isEmpty()/* &&
					!mld.password().isEmpty()*/ )
				{
					foreach( client * cl, _clients )
					{
						cl->logonUser( mld.userName(),
							mld.password(), mld.domain() );
					}
				}

			}
			break;

		case LogoutUser:
			if ( confirmLogout( _target ) )
			{
				foreach( client * cl, _clients )
				{
					cl->logoutUser();
				}
			}
			break;

		case Snapshot:
			foreach( client * cl, _clients )
			{
				cl->snapshot();
			}
			break;

		case PowerOn:
			foreach( client * cl, _clients )
			{
				cl->powerOn();
			}
			break;

		case Reboot:
			if ( confirmReboot( _target ) )
			{
				foreach( client * cl, _clients )
				{
					cl->reboot();
				}
			}
			break;

		case PowerDown:
			if ( confirmPowerDown( _target ) )
			{
				foreach( client * cl, _clients )
				{
					cl->powerDown();
				}
			}
			break;

		case ExecCmds:
			{
				QString cmds;
				cmdInputDialog cmd_input_dialog( cmds );

				if( cmd_input_dialog.exec() == QDialog::Accepted &&
					!cmds.isEmpty() )
				{
					foreach( client * cl, _clients )
					{
						cl->execCmds( cmds );
					}
				}
			}
			break;

		case RemoteScript:
			{
				QString script = dataExpanded( _clients );
				foreach( client * cl, _clients )
				{
					cl->execCmds( script );
				}
				break;
			}

		case LocalScript:
			{
				QString script = dataExpanded( _clients );
				QProcess::startDetached( script );
				break;
			}
	}
}




QString clientAction::dataExpanded( QVector<client *> _clients ) const
{
	static QRegExp s_reITALC_HOSTS( "\\$ITALC_HOSTS\\b" );

	QString script = data().toString();

	if ( script.contains( s_reITALC_HOSTS ) )
	{
		QStringList hosts;
		foreach ( client *cl, _clients )
		{
			hosts << cl->hostname();
		}
		script.replace( s_reITALC_HOSTS, hosts.join( " " ) );
	}

	return script;
}




void clientAction::process( QAction * _action, QVector<client *> _clients,
			targetGroup _target )
{
	clientAction * action = dynamic_cast<clientAction *>( _action );
	if ( action )
	{
		action->process( _clients, _target );
	}
}




bool clientAction::confirmLogout( targetGroup _target ) const
{
	QString question = ( _target == VisibleClients ?
		tr( "Are you sure want logout all users on all visible computers ?" ) :
		tr( "Are you sure want logout all users on all selected computers ?" ) );
	
	return QMessageBox::question( NULL, tr( "Logout user" ),
				question, QMessageBox::Yes, QMessageBox::No )
		== QMessageBox::Yes;
}




bool clientAction::confirmReboot( targetGroup _target ) const
{
	QString question = ( _target == VisibleClients ?
		tr( "Are you sure want to reboot all visible computers?" ) :
		tr( "Are you sure want to reboot all selected computers?" ) );

	return QMessageBox::question( NULL, tr( "Reboot computers" ),
				question, QMessageBox::Yes, QMessageBox::No )
		== QMessageBox::Yes;
}




bool clientAction::confirmPowerDown( targetGroup _target ) const
{
	QString question = ( _target == VisibleClients ?
		tr( "Are you sure want to power down all visible computers?" ) :
		tr( "Are you sure want to power down all selected computers?" ) );

	return QMessageBox::question( NULL, tr( "Reboot computers" ),
				question, QMessageBox::Yes, QMessageBox::No )
		== QMessageBox::Yes;
}




clientMenu::clientMenu( const QString & _title, const QList<QAction *> _actions,
			QWidget * _parent, const bool _fullMenu ) :
	QMenu( _title, _parent )
{
	setIcon( scaledIcon( "client.png" ) );

	foreach ( QAction * action, _actions )
	{
		clientAction * act = dynamic_cast<clientAction *>( action );
		if ( act && act->flags( clientAction::FullMenu ) && ! _fullMenu )
		{
			continue;
		}

		addAction( action );
	}
}



QMenu * clientMenu::createDefault( QWidget * _parent )
{
	QMenu * menu = new QMenu( _parent );

	menu->addAction( new clientAction( clientAction::Overview,
		scaledIcon( "overview_mode.png" ), tr( "Overview" ), menu ) );
	menu->addAction( new clientAction( clientAction::FullscreenDemo,
		scaledIcon( "fullscreen_demo.png" ), tr( "Fullscreen demo" ), menu ) );
	menu->addAction( new clientAction( clientAction::WindowDemo,
		scaledIcon( "window_demo.png" ), tr( "Window demo" ), menu ) );
	menu->addAction( new clientAction(	clientAction::Locked,
		scaledIcon( "locked.png" ), tr( "Locked display" ), menu ) );
	menu->addSeparator();

	menu->addAction( new clientAction( clientAction::ViewLive,
		scaledIcon( "viewmag.png" ), tr( "View live" ), menu,
		clientAction::FullMenu ) );
	menu->addAction( new clientAction( clientAction::RemoteControl,
		scaledIcon( "remote_control.png" ), tr( "Remote control" ), menu,
		clientAction::FullMenu ) );
	menu->addAction( new clientAction( clientAction::ClientDemo,
		scaledIcon( "client_demo.png" ), tr( "Let student show demo" ), menu,
		clientAction::FullMenu ) );

	menu->addAction( new clientAction( clientAction::SendTextMessage,
		scaledIcon( "text_message.png" ), tr( "Send text message" ), menu ) );
	menu->addSeparator();

	menu->addAction( new clientAction( clientAction::LogonUser,
		scaledIcon( "remotelogon.png" ), tr( "Logon user" ), menu ) );
	menu->addAction( new clientAction( clientAction::LogoutUser,
		scaledIcon( "logout.png" ), tr( "Logout user" ), menu ) );
	menu->addSeparator();

	menu->addAction( new clientAction( clientAction::Snapshot,
		scaledIcon( "snapshot.png" ), tr( "Take a snapshot" ), menu ) );
	menu->addSeparator();

	menu->addAction( new clientAction( clientAction::PowerOn,
		scaledIcon( "power_on.png" ), tr( "Power on" ), menu ) );
	menu->addAction( new clientAction( clientAction::Reboot,
		scaledIcon( "reboot.png" ), tr( "Reboot" ), menu ) );
	menu->addAction( new clientAction( clientAction::PowerDown,
		scaledIcon( "power_off.png" ), tr( "Power down" ), menu ) );
	menu->addAction( new clientAction( clientAction::ExecCmds,
		scaledIcon( "run.png" ), tr( "Execute commands" ), menu ) );

	return menu;
}








client::client( const QString & _hostname,
		const QString & _mac, const QString & _nickname,
		types _type, classRoom * _class_room,
					MainWindow * _main_window, int _id ) :
	QWidget( _main_window->workspace() ),
	m_mainWindow( _main_window ),
	m_connection( NULL ),
	m_clickPoint( -1, -1 ),
	m_origPos( -1, -1 ),
	m_hostname( _hostname ),
	m_nickname( _nickname ),
	m_mac( _mac ),
	m_type( _type ),
	m_mode( Mode_Overview ),
	m_user( "" ),
	m_makeSnapshot( FALSE ),
	m_state( State_Unkown ),
	m_syncMutex(),
	m_classRoomItem( NULL ),
	m_updateThread( NULL )
{
	new closeButton( this );

	if( _id <= 0 || clientFromID( _id ) != NULL )
	{
		_id = freeID();
	}
	s_clientIDs[_id] = this;


	m_classRoomItem = new classRoomItem( this, _class_room );

	m_connection = new ivsConnection( m_hostname,
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

	m_updateThread = new updateThread( this );
}




client::~client()
{
	quit();

	m_updateThread->wait(
#ifdef BUILD_WIN32
			2000
#else
			5000
#endif
				);
	if( !m_updateThread->isFinished() )
	{
		localSystem::sleep( 500 );
		m_updateThread->terminate();
	}

	delete m_updateThread;

//	m_syncMutex.lock();
	delete m_connection;
	m_connection = NULL;
//	m_syncMutex.unlock();

	delete m_classRoomItem;
}




void client::quit( void )
{
	changeMode( Mode_Overview );
	m_updateThread->quit();
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




void client::changeMode( const modes _new_mode )
{
	isdConnection * conn = m_mainWindow->localISD();

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
				conn->demoServerDenyClient( m_hostname );
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
				conn->demoServerAllowClient( m_hostname );
				m_updateThread->enqueueCommand(
						updateThread::Cmd_StartDemo,
	QList<QVariant>() << QString::number( conn->demoServerPort() )
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
		if( conn != NULL )
		{
			conn->demoServerDenyClient( m_hostname );
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
								m_hostname );
}




void client::update( void )
{
	// at least set tooltip with user-name if it is not displayed
	// in title-bar
	if( m_connection->state() == ivsConnection::Connected &&
		!m_mainWindow->getClassroomManager()->showUsername() )
	{
		if( toolTip() != m_user )
		{
			setToolTip( m_user );
		}
	}
	else
	{
		setToolTip( QString() );
	}

	m_state = currentState();
	QWidget::update();
}




bool client::userLoggedIn( void )
{
	QMutexLocker ml( &m_syncMutex );

	return( m_connection->state() == ivsConnection::Connected ||
		m_connection->reset( m_hostname ) ==
						ivsConnection::Connected );
}




void client::closeEvent( QCloseEvent * _ce )
{
	// make sure, client isn't forgotten by teacher after being hidden
	changeMode( Mode_Overview );
	hide();
	_ce->ignore();
}




void client::contextMenuEvent( QContextMenuEvent * )
{
	/* classRoomManager handles clientMenu */
	m_mainWindow->getClassroomManager()->clientMenuRequest();
}




void client::hideEvent( QHideEvent * )
{
	if( isMinimized() )
	{
		hide();
	}

	if( m_classRoomItem != NULL && m_mainWindow->isVisible() &&
						!m_mainWindow->isMinimized() )
	{
		m_classRoomItem->setVisible( FALSE );
	}

	m_mainWindow->getClassroomManager()->clientVisibleChanged();
}




void client::enlarge()
{
	if ( m_origSize.isValid() ) 
	{
		QWidget * workspace = m_mainWindow->workspace();
		QSize s = QSize( m_origSize );
		
		s.scale( workspace->parentWidget()->size(),
							Qt::KeepAspectRatio );
		setFixedSize( s );

		/* centralize */
		QSize offset = ( workspace->parentWidget()->size() - s ) / 2;
		move( offset.width() - workspace->x(),
			offset.height() - workspace->y() );

		raise();
	}
}




void client::zoom()
{
	m_origPos = pos();
	m_origSize = size();
	/* Delay zooming before we are sure that
	 * this is not just a fast click.
	 */
	QTimer::singleShot( 300, this, SLOT( enlarge() ) );
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
	classTreeWidget * tree = static_cast<classTreeWidget *>(
				m_classRoomItem->treeWidget() );

	tree->setCurrentItem( m_classRoomItem );

	if( _me->button() == Qt::LeftButton )
	{
		raise();
		m_clickPoint = _me->globalPos();
		m_origPos = pos();

		if ( ! ( _me->modifiers() & ( Qt::ControlModifier | Qt::ShiftModifier ) ))
		{
			tree->clearSelection();
		}
		m_classRoomItem->setSelected( ! m_classRoomItem->isSelected() );

		zoom();

		_me->ignore();
	}
	else if ( _me->button() == Qt::RightButton )
	{
		if ( ! m_classRoomItem->isSelected() ) {
			tree->clearSelection();
			m_classRoomItem->setSelected( TRUE );
		}
	}

	QWidget::mousePressEvent( _me );
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
		remoteControl();
	}
	else
	{
		viewLive();
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

	p.fillRect( 1, 1, width()-2, TITLE_HEIGHT-2,
			m_classRoomItem->isSelected() ?
				QColor( 96, 96, 96 ) :
						QColor( 224, 224, 224 ) );

	bool showUsername = m_mainWindow->getClassroomManager()->showUsername();
	const QString s = (showUsername && m_user != "") ? m_user :
		( name() + " (" + m_classRoomItem->parent()->text( 0 ) +
									")" );
	QFont f = p.font();
	f.setBold( TRUE );
	p.setFont( f );
	p.setPen( m_classRoomItem->isSelected() ? Qt::white : Qt::black );
	p.drawText( 10, TITLE_HEIGHT-7, s );

	if( m_connection->state() == ivsConnection::Connected &&
						m_mode == Mode_Overview )
	{
		p.drawImage( CONTENT_OFFSET, m_connection->scaledScreen() );
	}
	else
	{
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
	if( s.width() > 0 && s.height() > 0 )
	{
		p.drawImage( ( aw-s.width() ) / 2, height()-ah,
						fastQImage( *pm ).scaled( s ) );
	}

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
	findChild<closeButton*>()->move( width()-21, 3 );
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

	m_mainWindow->getClassroomManager()->clientVisibleChanged();
}




void client::reload()
{
	if( userLoggedIn() )
	{
		m_syncMutex.lock();

		m_connection->sendGetUserInformationRequest();
		if( m_connection->sendGetUserInformationRequest() )
		{
			// only send a framebuffer-update-request if client
			// is in (over)view-mode
			m_connection->handleServerMessages(
						m_mode == Mode_Overview );
		}

		m_user = m_connection->user();
		m_syncMutex.unlock();
	}
	else
	{
		m_user = "";
	}

	if( m_classRoomItem )
	{
		m_classRoomItem->setUser( m_user );
	}
}




void client::clientDemo()
{
	classroomManager * cm = m_mainWindow->getClassroomManager();
	cm->changeGlobalClientMode( Mode_Overview );

	QVector<client *> vc = cm->visibleClients();

	foreach( client * cl, vc )
	{
		if( cl != this )
		{
			cl->changeMode( Mode_FullscreenDemo );
		}
	}

	//m_mainWindow->checkModeButton( client::Mode_FullscreenDemo );

	m_mainWindow->remoteControlDisplay( m_hostname, TRUE, TRUE );
}




void client::viewLive()
{
	changeMode( Mode_Overview );

	m_mainWindow->remoteControlDisplay( m_hostname, TRUE );
}




void client::remoteControl()
{
	changeMode( Mode_Overview );

	m_mainWindow->remoteControlDisplay( m_hostname );
}




void client::sendTextMessage( const QString & _msg )
{
	m_updateThread->enqueueCommand(
			updateThread::Cmd_SendTextMessage, _msg );
}




void client::logonUser( const QString & _username, const QString & _password,
			const QString & _domain )
{
	m_updateThread->enqueueCommand( updateThread::Cmd_LogonUser,
		_username + "*" + _password + "*" + _domain );
}




void client::logoutUser()
{
	m_updateThread->enqueueCommand( updateThread::Cmd_LogoutUser );
}




void client::snapshot()
{
	m_makeSnapshot = TRUE;
}





void client::powerOn()
{
	m_mainWindow->localISD()->wakeOtherComputer( m_mac );
}




void client::reboot()
{
	m_updateThread->enqueueCommand( updateThread::Cmd_Reboot );
}





void client::powerDown()
{
	m_updateThread->enqueueCommand( updateThread::Cmd_PowerDown );
}




void client::execCmds( const QString & _cmds )
{
	m_updateThread->enqueueCommand( updateThread::Cmd_ExecCmds, _cmds );
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

	if( m_client->m_classRoomItem->isVisible() )
	{
		if( !m_client->m_mainWindow->remoteControlRunning() &&
						!MainWindow::atExit() )
		{
			m_client->reload();
		}
	}
	else if( m_client->m_connection->state() == ivsConnection::Connected
						&& !MainWindow::atExit() )
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

