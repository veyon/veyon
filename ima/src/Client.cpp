/*
 * Client.cpp - code for client-windows, which are displayed in several
 *              instances in the main-window of iTALC
 *
 * Copyright (c) 2004-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include "Client.h"
#include "ItalcVncConnection.h"
#include "ItalcConfiguration.h"
#include "ItalcCoreConnection.h"
#include "ClassroomManager.h"
#include "RunCommandsDialog.h"
#include "LocalSystem.h"
#include "Snapshot.h"
#include "Dialogs.h"
#include "DecoratedMessageBox.h"



const QSize DEFAULT_CLIENT_SIZE( 256, 192 );
const int DECO_WIDTH = 4;
const int TITLE_HEIGHT = 23;
const QPoint CONTENT_OFFSET( DECO_WIDTH, DECO_WIDTH + TITLE_HEIGHT ); 
const QSize CONTENT_SIZE_SUB( 2*DECO_WIDTH, 2*DECO_WIDTH + TITLE_HEIGHT ); 


// resolve static symbols...
QHash<int, Client *> Client::s_clientIDs;


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




ClientAction::ClientAction( Type _type, QObject * _parent, int _flags ) :
	QAction( _parent ),
	m_type( _type ),
	m_flags( _flags )
{
}




ClientAction::ClientAction( Type _type, const QIcon & _icon, const QString & _text,
				QObject * _parent, int _flags ) :
	QAction( _icon, _text, _parent ),
	m_type( _type ),
	m_flags( _flags )
{
}




void ClientAction::process( QVector<Client *> _clients, TargetGroup _target )
{
	switch ( m_type )
	{
		case Overview:
			foreach( Client * cl, _clients )
			{
				cl->changeMode( Client::Mode_Overview );
			}
			break;
			
		case FullscreenDemo:
			foreach( Client * cl, _clients )
			{
				cl->changeMode( Client::Mode_FullscreenDemo );
			}
			break;
			
		case WindowDemo:
			foreach( Client * cl, _clients )
			{
				cl->changeMode( Client::Mode_WindowDemo );
			}
			break;
			
		case Locked:
			foreach( Client * cl, _clients )
			{
				cl->changeMode( Client::Mode_Locked );
			}
			break;
			
		case ViewLive:
			foreach( Client * cl, _clients )
			{
				cl->viewLive();
			}
			break;
			
		case RemoteControl:
			foreach( Client * cl, _clients )
			{
				cl->remoteControl();
			}
			break;
			
		case ClientDemo:
			foreach( Client * cl, _clients )
			{
				cl->clientDemo();
			}
			break;
			
		case SendTextMessage:
			{
				QString msg;
				TextMessageDialog tmd( msg, NULL );

				if( tmd.exec() == QDialog::Accepted &&
					!msg.isEmpty() )
				{ 
					foreach( Client * cl, _clients )
					{
						cl->sendTextMessage( msg );
					}
				}
			}

			break;

#if 0
		case LogonUser:
			{
				RemoteLogonDialog mld( NULL );

				if( mld.exec() == QDialog::Accepted &&
					!mld.userName().isEmpty()/* &&
					!mld.password().isEmpty()*/ )
				{
					foreach( Client * cl, _clients )
					{
						cl->logonUser( mld.userName(),
							mld.password(), mld.domain() );
					}
				}

			}
			break;
#endif

		case LogoutUser:
			if ( confirmLogout( _target ) )
			{
				foreach( Client * cl, _clients )
				{
					cl->logoutUser();
				}
			}
			break;

		case Snapshot:
			foreach( Client * cl, _clients )
			{
				cl->snapshot();
			}
			break;

		case PowerOn:
			foreach( Client * cl, _clients )
			{
				cl->powerOn();
			}
			break;

		case Reboot:
			if ( confirmReboot( _target ) )
			{
				foreach( Client * cl, _clients )
				{
					cl->reboot();
				}
			}
			break;

		case PowerDown:
			if ( confirmPowerDown( _target ) )
			{
				foreach( Client * cl, _clients )
				{
					cl->powerDown();
				}
			}
			break;

		case ExecCmds:
			{
				QString cmds;
				RunCommandsDialog cmd_input_dialog( cmds, NULL );

				if( cmd_input_dialog.exec() == QDialog::Accepted &&
					!cmds.isEmpty() )
				{
					foreach( Client * cl, _clients )
					{
						cl->execCmds( cmds );
					}
				}
			}
			break;

		case RemoteScript:
			{
				QString script = dataExpanded( _clients );
				foreach( Client * cl, _clients )
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




QString ClientAction::dataExpanded( QVector<Client *> _clients ) const
{
	static QRegExp s_reITALC_HOSTS( "\\$ITALC_HOSTS\\b" );

	QString script = data().toString();

	if ( script.contains( s_reITALC_HOSTS ) )
	{
		QStringList hosts;
		foreach ( Client *cl, _clients )
		{
			hosts << cl->hostname();
		}
		script.replace( s_reITALC_HOSTS, hosts.join( " " ) );
	}

	return script;
}




void ClientAction::process( QAction * _action, QVector<Client *> _clients,
			TargetGroup _target )
{
	ClientAction * action = dynamic_cast<ClientAction *>( _action );
	if ( action )
	{
		action->process( _clients, _target );
	}
}




bool ClientAction::confirmLogout( TargetGroup _target ) const
{
	QString question = ( _target == VisibleClients ?
		tr( "Are you sure want logout all users on all visible computers ?" ) :
		tr( "Are you sure want logout all users on all selected computers ?" ) );
	
	return QMessageBox::question( NULL, tr( "Logout user" ),
				question, QMessageBox::Yes, QMessageBox::No )
		== QMessageBox::Yes;
}




bool ClientAction::confirmReboot( TargetGroup _target ) const
{
	QString question = ( _target == VisibleClients ?
		tr( "Are you sure want to reboot all visible computers?" ) :
		tr( "Are you sure want to reboot all selected computers?" ) );

	return QMessageBox::question( NULL, tr( "Reboot computers" ),
				question, QMessageBox::Yes, QMessageBox::No )
		== QMessageBox::Yes;
}




bool ClientAction::confirmPowerDown( TargetGroup _target ) const
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
		ClientAction * act = dynamic_cast<ClientAction *>( action );
		if ( act && act->flags( ClientAction::FullMenu ) && ! _fullMenu )
		{
			continue;
		}

		addAction( action );
	}
}



QMenu * clientMenu::createDefault( QWidget * _parent )
{
	QMenu * menu = new QMenu( _parent );

	menu->addAction( new ClientAction( ClientAction::Overview,
		scaledIcon( "overview_mode.png" ), tr( "Overview" ), menu ) );
	menu->addAction( new ClientAction( ClientAction::FullscreenDemo,
		scaledIcon( "fullscreen_demo.png" ), tr( "Fullscreen demo" ), menu ) );
	menu->addAction( new ClientAction( ClientAction::WindowDemo,
		scaledIcon( "window_demo.png" ), tr( "Window demo" ), menu ) );
	menu->addAction( new ClientAction(	ClientAction::Locked,
		scaledIcon( "locked.png" ), tr( "Locked display" ), menu ) );
	menu->addSeparator();

	menu->addAction( new ClientAction( ClientAction::ViewLive,
		scaledIcon( "viewmag.png" ), tr( "View live" ), menu,
		ClientAction::FullMenu ) );
	menu->addAction( new ClientAction( ClientAction::RemoteControl,
		scaledIcon( "remote_control.png" ), tr( "Remote control" ), menu,
		ClientAction::FullMenu ) );
	menu->addAction( new ClientAction( ClientAction::ClientDemo,
		scaledIcon( "client_demo.png" ), tr( "Let student show demo" ), menu,
		ClientAction::FullMenu ) );

	menu->addAction( new ClientAction( ClientAction::SendTextMessage,
		scaledIcon( "text_message.png" ), tr( "Send text message" ), menu ) );
	menu->addSeparator();

/*	menu->addAction( new ClientAction( ClientAction::LogonUser,
		scaledIcon( "remotelogon.png" ), tr( "Logon user" ), menu ) );*/
	menu->addAction( new ClientAction( ClientAction::LogoutUser,
		scaledIcon( "logout.png" ), tr( "Logout user" ), menu ) );
	menu->addSeparator();

	menu->addAction( new ClientAction( ClientAction::Snapshot,
		scaledIcon( "snapshot.png" ), tr( "Take a snapshot" ), menu ) );
	menu->addSeparator();

	menu->addAction( new ClientAction( ClientAction::PowerOn,
		scaledIcon( "power_on.png" ), tr( "Power on" ), menu ) );
	menu->addAction( new ClientAction( ClientAction::Reboot,
		scaledIcon( "reboot.png" ), tr( "Reboot" ), menu ) );
	menu->addAction( new ClientAction( ClientAction::PowerDown,
		scaledIcon( "power_off.png" ), tr( "Power down" ), menu ) );
	menu->addAction( new ClientAction( ClientAction::ExecCmds,
		scaledIcon( "run.png" ), tr( "Execute commands" ), menu ) );

	return menu;
}








Client::Client( const QString & _hostname,
		const QString & _mac, const QString & _nickname,
		Types _type, classRoom * _class_room,
					MainWindow * mainWindow, int _id ) :
	QWidget( mainWindow->workspace() ),
	m_mainWindow( mainWindow ),
	m_connection( NULL ),
	m_vncConn( NULL ),
	m_framebufferUpdated( false ),
	m_clickPoint( -1, -1 ),
	m_origPos( -1, -1 ),
	m_hostname( _hostname ),
	m_nickname( _nickname ),
	m_mac( _mac ),
	m_type( _type ),
	m_mode( Mode_Overview ),
	m_user(),
	m_takeSnapshot( false ),
	m_state( State_Unkown ),
	m_classRoomItem( NULL )
{
	new closeButton( this );

	if( _id <= 0 || clientFromID( _id ) != NULL )
	{
		_id = freeID();
	}
	s_clientIDs[_id] = this;


	m_classRoomItem = new classRoomItem( this, _class_room );

	m_vncConn = new ItalcVncConnection( this );
	m_vncConn->setHost( m_hostname );
	m_vncConn->setQuality( ItalcVncConnection::ThumbnailQuality );
	m_vncConn->setFramebufferUpdateInterval(
				m_mainWindow->getClassroomManager()->updateInterval() );

	// set a flag so we only update the view if there were some updates
	connect( m_vncConn, SIGNAL( imageUpdated( int, int, int, int ) ),
				this, SLOT( setUpdateFlag() ) );

	m_connection = new ItalcCoreConnection( m_vncConn );

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

	reload();
}




Client::~Client()
{
	changeMode( Mode_Overview );

	delete m_connection;
	m_connection = NULL;

	delete m_vncConn;
	m_vncConn = NULL;

	delete m_classRoomItem;
}




int Client::id( void ) const
{
	QHash<int, Client *>::const_iterator it;
	for( it = s_clientIDs.begin(); it != s_clientIDs.end(); ++it )
	{
		if( it.value() == this )
		{
			return( it.key() );
		}
	}
	return( 0 );
}




Client * Client::clientFromID( int _id )
{
	if( s_clientIDs.contains( _id ) )
	{
		return( s_clientIDs[_id] );
	}
	return( NULL );
}




int Client::freeID( void )
{
	const int ID_MAX = 1 << 20;

	int id;
	while( s_clientIDs.contains( id = static_cast<int>( (float) rand() /
						RAND_MAX * ID_MAX + 1 ) ) )
	{
	}
	return( id );
}




void Client::changeMode( const Modes _new_mode )
{
	if( _new_mode != m_mode )
	{
		switch( m_mode )
		{
			case Mode_Overview:
			case Mode_Unknown:
				break;
			case Mode_FullscreenDemo:
			case Mode_WindowDemo:
				m_mainWindow->localICA()->demoServerUnallowHost( m_hostname );
				m_connection->stopDemo();
				break;
			case Mode_Locked:
				m_connection->unlockScreen();
				break;
		}
		switch( m_mode = _new_mode )
		{
			case Mode_Overview:
			case Mode_Unknown:
				break;
			case Mode_FullscreenDemo:
			case Mode_WindowDemo:
				m_mainWindow->localICA()->demoServerAllowHost( m_hostname );
				m_connection->startDemo(
								QString(),// let client guess IP from connection
								ItalcCore::config->demoServerPort(),
								m_mode == Mode_FullscreenDemo );
				break;
			case Mode_Locked:
				m_connection->lockScreen();
				break;
		}
	}
	// if connection was lost while sending commands such as stop-demo,
	// there should be a way for switching back into normal mode, that's
	// why we offer this lines
	else if( m_mode == Mode_Overview )
	{
	/*	if( conn != NULL )
		{
			conn->demoServerDenyClient( m_hostname );
		}*/
		m_connection->stopDemo();
		m_connection->unlockScreen();
	}
}




void Client::setClassRoom( classRoom * _cr )
{
	delete m_classRoomItem;

	m_classRoomItem = new classRoomItem( this, _cr );
}




void Client::resetConnection( void )
{
	m_connection->vncConnection()->reset( m_hostname );
}




void Client::update( void )
{
	// at least set tooltip with user-name if it is not displayed
	// in title-bar
	if( m_connection->isConnected() &&
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




bool Client::userLoggedIn( void )
{
	if( m_connection->isConnected() )
	{
		return true;
	}
	m_connection->vncConnection()->reset( m_hostname );
	return false;
}




void Client::closeEvent( QCloseEvent * _ce )
{
	// make sure, client isn't forgotten by teacher after being hidden
	changeMode( Mode_Overview );
	hide();
	_ce->ignore();
}




void Client::contextMenuEvent( QContextMenuEvent * )
{
	/* classRoomManager handles clientMenu */
	m_mainWindow->getClassroomManager()->clientMenuRequest();
}




void Client::hideEvent( QHideEvent * )
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




void Client::enlarge()
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




void Client::zoom()
{
	m_origPos = pos();
	m_origSize = size();
	/* Delay zooming before we are sure that
	 * this is not just a fast click.
	 */
	QTimer::singleShot( 300, this, SLOT( enlarge() ) );
}




void Client::zoomBack()
{
	if ( m_origSize.isValid() )
	{	
		move( m_origPos );
		setFixedSize( m_origSize );
		/* reset value: */
		m_origSize = QSize();
	}
}




void Client::mousePressEvent( QMouseEvent * _me )
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




void Client::mouseMoveEvent( QMouseEvent * _me )
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




void Client::mouseReleaseEvent( QMouseEvent * _me )
{
	zoomBack();
	m_clickPoint = QPoint( -1, -1 );
	QWidget::mouseReleaseEvent( _me );
}




void Client::mouseDoubleClickEvent( QMouseEvent * _me )
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




void Client::paintEvent( QPaintEvent * _pe )
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

	if( m_mode == Mode_Overview && m_connection->isConnected() &&
			m_connection->vncConnection()->framebufferInitialized() )
	{
		p.drawImage( CONTENT_OFFSET, m_connection->
					vncConnection()->scaledScreen() );
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
	p.drawImage( ( aw-s.width() ) / 2, height()-ah,
						FastQImage( *pm ).scaled( s ) );

	p.setPen( QColor( 0, 0, 0 ) );
	p.drawText( QRect( 5, height()-r.height()-10, aw - 10,
						r.height() ),
				Qt::TextWordWrap | Qt::AlignCenter, msg );
	}

	if( m_takeSnapshot )
	{
		Snapshot().take( m_connection->vncConnection(), m_user );
		m_takeSnapshot = false;
	}

}




void Client::resizeEvent( QResizeEvent * _re )
{
	findChild<closeButton*>()->move( width()-21, 3 );
	m_connection->vncConnection()->setScaledSize( size() -
							CONTENT_SIZE_SUB );
	m_connection->vncConnection()->rescaleScreen();
	QWidget::resizeEvent( _re );
}





void Client::showEvent( QShowEvent * )
{
	if( m_classRoomItem != NULL )
	{
		m_classRoomItem->setVisible( TRUE );
	}

	m_mainWindow->getClassroomManager()->clientVisibleChanged();
}




void Client::reload()
{
	QTimer::singleShot(
				m_mainWindow->getClassroomManager()->updateInterval(),
				this,
				SLOT( reload() ) );
	if( !isVisible() )
	{
		if( m_connection->vncConnection()->isRunning() )
		{
			m_connection->vncConnection()->stop();
			update();
		}
		return;
	}

	if( userLoggedIn() )
	{
		m_connection->sendGetUserInformationRequest();
		if( m_connection->user() != m_user )
		{
			m_user = m_connection->user();
			update();
		}
		if( m_framebufferUpdated )
		{
			m_framebufferUpdated = false;
			update();
		}
	}
	else
	{
		if( !m_user.isEmpty() )
		{
			m_user = QString();
			update();
		}
	}

	if( m_classRoomItem )
	{
		m_classRoomItem->setUser( m_user );
	}
}




void Client::setUpdateFlag()
{
	m_framebufferUpdated = true;
}




void Client::clientDemo()
{
	ClassroomManager * cm = m_mainWindow->getClassroomManager();
	cm->changeGlobalClientMode( Mode_Overview );

	QVector<Client *> vc = cm->visibleClients();

	foreach( Client * cl, vc )
	{
		if( cl != this )
		{
			cl->changeMode( Mode_FullscreenDemo );
		}
	}

	//m_mainWindow->checkModeButton( Client::Mode_FullscreenDemo );

	m_mainWindow->remoteControlDisplay( m_hostname, TRUE, TRUE );
}




void Client::viewLive()
{
	changeMode( Mode_Overview );

	m_mainWindow->remoteControlDisplay( m_hostname, TRUE );
}




void Client::remoteControl()
{
	changeMode( Mode_Overview );

	m_mainWindow->remoteControlDisplay( m_hostname );
}




void Client::sendTextMessage( const QString & _msg )
{
	m_connection->displayTextMessage( _msg );
}




/*void Client::logonUser( const QString & _username, const QString & _password,
			const QString & _domain )
{
	m_connection->logonUser( _username, _password, _domain );
}*/




void Client::logoutUser()
{
	m_connection->logoutUser();
}




void Client::snapshot()
{
	m_takeSnapshot = true;
}





void Client::powerOn()
{
	// we have to send the wake-on-LAN packets with root privileges,
	// therefore let the local ICA do the job (as it usually is running
	// with higher privileges)
	m_mainWindow->localICA()->powerOnComputer( m_mac );
}




void Client::reboot()
{
	m_connection->restartComputer();
}





void Client::powerDown()
{
	m_connection->powerDownComputer();
}




void Client::execCmds( const QString & _cmds )
{
	m_connection->execCmds( _cmds );
}




Client::States Client::currentState( void ) const
{
	switch( m_mode )
	{
		case Mode_Overview:
			if( m_connection->isConnected() )
			{
				return State_Overview;
			}
		/*	else if( m_connection->state() ==
					ivsConnection::ConnectionRefused )
			{
				return( State_NoUserLoggedIn );
			}*/
			return State_Unreachable;

		case Mode_FullscreenDemo:
		case Mode_WindowDemo:
			return State_Demo;

		case Mode_Locked:
			return State_Locked;

		default:
			break;
	}

	return State_Unkown;
}



