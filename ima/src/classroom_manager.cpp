/*
 * classroom_manager.cpp - implementation of classroom-manager
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>


#include <QtCore/QDateTime>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtGui/QButtonGroup>
#include <QtGui/QCloseEvent>
#include <QtGui/QFileDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QInputDialog>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QPixmap>
#include <QtGui/QSplashScreen>
#include <QtGui/QSplitter>
#include <QtGui/QWorkspace>
#include <QtNetwork/QHostInfo>


#include "main_window.h"
#include "classroom_manager.h"
#include "client.h"
#include "dialogs.h"
#include "cmd_input_dialog.h"
#include "italc_side_bar.h"
#include "local_system.h"
#include "tool_button.h"
#include "messagebox.h"


template<typename T>
inline T roundCorrect( T _val )
{
	if( _val - floor( _val ) < 0.5 )
	{
		return( floor( _val ) );
	}
	return( ceil( _val ) );
}




const int widths[] = { 128, 192, 256, 320, 384, 448, 512, 0 };



QPixmap * classRoomItem::s_clientPixmap = NULL;
QPixmap * classRoomItem::s_clientObservedPixmap = NULL;



classroomManager::classroomManager( mainWindow * _main_window,
							QWidget * _parent ) :
	sideBarWidget( QPixmap( ":/resources/classroom_manager.png" ),
			tr( "Classroom-Manager" ),
			tr( "Use this workspace to manage your computers and "
				"classrooms in an easy way." ),
			_main_window, _parent ),
	m_personalConfiguration( localSystem::personalConfigPath() ),
	m_globalClientConfiguration( localSystem::globalConfigPath() ),
	m_quickSwitchMenu( new QMenu( this ) ),
	m_qsmClassRoomSeparator( m_quickSwitchMenu->addSeparator() ),
	m_globalClientMode( client::Mode_Overview ),
	m_clientUpdateInterval( 1 )
{
	// some code called out of this function relies on m_classroomManager
	// which actually is assigned after this function returns
	_main_window->m_classroomManager = this;

	m_view = new QTreeWidget( contentParent() );
	contentParent()->layout()->addWidget( m_view );
	m_view->setWhatsThis( tr( "This is where computers and classrooms are "
					"managed. You can add computers or "
					"classrooms by clicking right "
					"in this list." ) );

	QStringList columns;
	columns << tr( "Classrooms/computers" ) << tr( "IP-address" );
	m_view->setHeaderLabels( columns );
	m_view->setSelectionMode( QTreeWidget::ExtendedSelection );
	m_view->setIconSize( QSize( 22, 22 ) );
	m_view->header()->resizeSection( m_view->header()->logicalIndex( 0 ),
									200 );
	m_view->setSortingEnabled( TRUE );
	m_view->setRootIsDecorated( TRUE );
	m_view->sortItems( 0, Qt::AscendingOrder );
//	m_view->setShowToolTips( TRUE );
	m_view->setWindowTitle( tr( "Classroom-Manager" ) );
	m_view->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( m_view, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ),
		this, SLOT( itemDoubleClicked( QTreeWidgetItem *, int ) ) );
	connect( m_view, SIGNAL( customContextMenuRequested( const QPoint & ) ),
			this, SLOT( contextMenuRequest( const QPoint & ) ) );


	QFont f;
	f.setPixelSize( 12 );
	QLabel * help_txt = new QLabel(
		tr( "\nUse the context-menu (right mouse-button) to add/remove "
			"computers and/or classrooms.\n\n"
			"Once you did so you can show or hide clients by "
			"double-clicking them.\n\nFurther actions can be "
			"found in the context-menu of a computer or "
			"classroom.\n" ),
							contentParent() );
	contentParent()->layout()->addWidget( help_txt );
	help_txt->setWordWrap( TRUE );
	help_txt->setFont( f );

	loadGlobalClientConfig();
	loadPersonalConfig();

	QAction * hide_teacher_clients =
		m_quickSwitchMenu->addAction( tr( "Hide teacher clients" ) );
	connect( hide_teacher_clients, SIGNAL( triggered( bool ) ),
					this, SLOT( hideTeacherClients() ) );

	show();
}




classroomManager::~classroomManager()
{
}




void classroomManager::doCleanupWork( void )
{
	while( m_clientsToRemove.size() )
	{
		delete m_clientsToRemove.first();
		m_clientsToRemove.erase( m_clientsToRemove.begin() );
	}

	while( m_classRoomsToRemove.size() )
	{
		QVector<classRoom *>::iterator it =
				qFind( m_classRooms.begin(), m_classRooms.end(),
						m_classRoomsToRemove.first() );
		if( it != m_classRooms.end() )
		{
			m_classRooms.erase( it );
		}
		delete m_classRoomsToRemove.first();
		m_classRoomsToRemove.erase( m_classRoomsToRemove.begin() );
	}
	//savePersonalConfig();
	//saveGlobalClientConfig();
}




void classroomManager::saveGlobalClientConfig( void )
{
	QDomDocument doc( "italc-config-file" );

	QDomElement italc_config = doc.createElement( "globalclientconfig" );
	italc_config.setAttribute( "version", VERSION );
	doc.appendChild( italc_config );

	QDomElement root = doc.createElement( "body" );
	italc_config.appendChild( root );

	for( int i = 0; i < m_view->topLevelItemCount(); ++i )
	{
		saveSettingsOfChildren( doc, root, m_view->topLevelItem( i ),
									TRUE );
	}

	QString xml = "<?xml version=\"1.0\"?>\n" + doc.toString( 2 );
/*	if( mainWindow::ensureConfigPathExists() == FALSE )
	{
		qFatal( QString( "Could not read/write or create directory %1!"
					"For running iTALC, make sure you have "
					"write-access to your home-directory "
					"and to %1 (if already existing)."
					).arg( ITALC_CONFIG_PATH
						).toAscii().constData() );
	}*/

	QFile( m_globalClientConfiguration + ".bak" ).remove();
	QFile( m_globalClientConfiguration ).copy( m_globalClientConfiguration +
									".bak" );
	QFile outfile( m_globalClientConfiguration );
	outfile.open( QFile::WriteOnly | QFile::Truncate );

	outfile.write( xml.toAscii() );
	outfile.close();
}




void classroomManager::savePersonalConfig( void )
{
	QDomDocument doc( "italc-config-file" );

	QDomElement italc_config = doc.createElement( "personalconfig" );
	italc_config.setAttribute( "version", VERSION );
	doc.appendChild( italc_config );

	QDomElement head = doc.createElement( "head" );
	italc_config.appendChild( head );

	QDomElement globalsettings = doc.createElement( "globalsettings" );
	globalsettings.setAttribute( "client-update-interval",
						m_clientUpdateInterval );
	globalsettings.setAttribute( "win-width", getMainWindow()->width() );
	globalsettings.setAttribute( "win-height", getMainWindow()->height() );
	globalsettings.setAttribute( "ismaximized", getMainWindow()->isMaximized() );
	globalsettings.setAttribute( "opened-tab",
				getMainWindow()->m_sideBar->openedTab() );

	globalsettings.setAttribute( "wincfg", QString(
				getMainWindow()->saveState().toBase64() ) );

	globalsettings.setAttribute( "demo-net-iface", __demo_network_interface );
	globalsettings.setAttribute( "demo-master-ip", __demo_master_ip );
	globalsettings.setAttribute( "defaultdomain", __default_domain );
	globalsettings.setAttribute( "demoquality", __demo_quality );
	globalsettings.setAttribute( "role", __role );
	globalsettings.setAttribute( "notooltips",
					toolButton::toolTipsDisabled() );
	globalsettings.setAttribute( "clientdoubleclickaction",
						m_clientDblClickAction );

	head.appendChild( globalsettings );


	QDomElement root = doc.createElement( "body" );
	italc_config.appendChild( root );

	for( int i = 0; i < m_view->topLevelItemCount(); ++i )
	{
		saveSettingsOfChildren( doc, root, m_view->topLevelItem( i ),
									FALSE );
	}


	QString xml = "<?xml version=\"1.0\"?>\n" + doc.toString( 2 );
	if( mainWindow::ensureConfigPathExists() == FALSE )
	{
		qWarning( QString( "Could not read/write or create directory %1!"
					"For running iTALC, make sure you have "
					"write-access to your home-directory "
					"and to %1 (if already existing)."
				).arg( localSystem::personalConfigDir()
						).toAscii().constData() );
	}

	QFile( m_personalConfiguration + ".bak" ).remove();
	QFile( m_personalConfiguration ).copy( m_personalConfiguration + ".bak" );
	QFile outfile( m_personalConfiguration );
	outfile.open( QFile::WriteOnly | QFile::Truncate );

	outfile.write( xml.toAscii() );
	outfile.close();
}




void classroomManager::saveSettingsOfChildren( QDomDocument & _doc,
						QDomElement & _root,
						QTreeWidgetItem * _parent,
							bool _is_global_config )
{
	QDomElement classroom = _doc.createElement( "classroom" );
	classroom.setAttribute( "name", _parent->text( 0 ) );
	_root.appendChild( classroom );


	for( int i = 0; i < _parent->childCount(); ++i )
	{
		QTreeWidgetItem * lvi = _parent->child( i );
		if( lvi->childCount() ||
				dynamic_cast<classRoom *>( lvi ) != NULL )
		{
			saveSettingsOfChildren( _doc, classroom, lvi,
							_is_global_config );
		}
		else
		{
			if( dynamic_cast<classRoomItem *>( lvi ) != NULL )
			{
				client * c = dynamic_cast<classRoomItem *>(
							lvi )->getClient();
				QDomElement client_element = _doc.createElement(
								"client" );
				client_element.setAttribute( "id", c->id() );
				if( _is_global_config )
				{
					client_element.setAttribute( "name",
								c->name() );
					client_element.setAttribute( "localip",
								c->localIP() );
					client_element.setAttribute( "remoteip",
								c->remoteIP() );
					client_element.setAttribute( "mac",
								c->mac() );
					client_element.setAttribute( "type",
								c->type() );
				}
				else
				{
					client_element.setAttribute( "visible",
						c->isVisible() ? "yes" : "no" );
						client_element.setAttribute(
			"x", QString::number( c->parentWidget()->pos().x() ) );
 						client_element.setAttribute(
			"y", QString::number( c->parentWidget()->pos().y() ) );
						client_element.setAttribute(
			"w", QString::number( c->width() ) );
 						client_element.setAttribute(
			"h", QString::number( c->height() ) );
				}
				classroom.appendChild( client_element );
			}
		}
	}
}




// routine that returns m_view of all visible clients
QVector<client *> classroomManager::visibleClients( void ) const
{
	QVector<client *> vc;
	for( int i = 0; i < m_view->topLevelItemCount(); ++i )
	{
		getVisibleClients( m_view->topLevelItem( i ), vc );
	}

	return( vc );
}




void classroomManager::getVisibleClients( QTreeWidgetItem * _p,
						QVector<client *> & _vc )
{
	classRoomItem * l = NULL;

	for( int i = 0; i < _p->childCount(); ++i )
	{
		QTreeWidgetItem * lvi = _p->child( i );
		if( lvi->childCount() )
		{
			getVisibleClients( lvi, _vc );
		}
		else if( ( l = dynamic_cast<classRoomItem *>( lvi ) ) != NULL &&
					l->getClient()->isVisible() )
		{
			_vc.push_back( l->getClient() );
		}
	}
}




void classroomManager::getHeaderInformation( const QDomElement & _header )
{
	QDomNode node = _header.firstChild();

	while( !node.isNull() )
	{
		if( node.isElement() && node.nodeName() == "globalsettings" )
		{
			m_clientUpdateInterval = node.toElement().attribute(
					"client-update-interval" ).toInt();
			if( node.toElement().attribute( "win-width" ) !=
								QString::null &&
				node.toElement().attribute( "win-height" ) !=
								QString::null )
			{
				getMainWindow()->resize(
			node.toElement().attribute( "win-width" ).toInt(),
			node.toElement().attribute( "win-height" ).toInt() );
			}
			if( node.toElement().attribute( "opened-tab" ) !=
								QString::null )
			{
				getMainWindow()->m_openedTabInSideBar =
						node.toElement().attribute(
							"opened-tab" ).toInt();
			}
			if( node.toElement().attribute( "ismaximized" ).toInt() > 0 )
			{
				getMainWindow()->setWindowState( getMainWindow()->windowState() |
										Qt::WindowMaximized );
			}
			if( node.toElement().attribute( "wincfg" ) !=
								QString::null )
			{
				m_winCfg = node.toElement().attribute(
								"wincfg" );
			}
			// for now we only set the network-interface over which
			// the demo should run
			__demo_network_interface = node.toElement().
						attribute( "demo-net-iface" );

			__demo_master_ip = node.toElement().
						attribute( "demo-master-ip" );

			__demo_quality = node.toElement().
					attribute( "demoquality" ).toInt();

			__default_domain = node.toElement().
						attribute( "defaultdomain" );

			__role = static_cast<ISD::userRoles>(
				node.toElement().attribute( "role" ).toInt() );
			if( __role <= ISD::RoleNone ||
						__role >= ISD::RoleCount )
			{
				__role = ISD::RoleTeacher;
			}
			toolButton::setToolTipsDisabled(
				node.toElement().attribute( "notooltips" ).
								toInt() );
			m_clientDblClickAction = node.toElement().attribute(
					"clientdoubleclickaction" ).toInt();
			// if the attr did not exist, we got zero as value,
			// which is not acceptable
			if( m_clientUpdateInterval < 1 )
			{
				m_clientUpdateInterval = 2;
			}
		}
		node = node.nextSibling();
        }
}




void classroomManager::loadTree( classRoom * _parent_item,
					const QDomElement & _parent_element,
						bool _is_global_config )
{
	for( QDomNode node = _parent_element.firstChild();
						node.isNull() == FALSE;
						node = node.nextSibling() )
	{
		if( node.isElement() == FALSE )
		{
			continue;
		}

		if( node.nodeName() == "classroom" )
		{
			classRoom * cur_item = NULL;
			if( _is_global_config )
			{
				// add new classroom
				QString name = node.toElement().attribute(
								"name" );
				if( _parent_item == NULL )
				{
					cur_item = new classRoom( name, this,
								m_view );
				}
				else
				{
					cur_item = new classRoom( name, this,
								_parent_item );
				}

				m_classRooms.push_back( cur_item );
			}

			// recursive build of the tree
			loadTree( cur_item, node.toElement(),
							_is_global_config );
		}
		else if( node.nodeName() == "client" )
		{
			if( _is_global_config )
			{
                                client * c;
				// add new client
				QString local_ip = node.toElement().attribute(
								"localip" );

				// compat-code - to be removed with final 1.0.0
				if( local_ip == QString::null )
				{
					local_ip = node.toElement().attribute(
									"ip" );
				}
				QString remote_ip = node.toElement().attribute(
								"remoteip" );
				QString mac = node.toElement().attribute(
									"mac" );
				QString name = node.toElement().attribute(
								"name" );

	c = new client( local_ip, remote_ip, mac, name,
				(client::types) node.toElement().
						attribute( "type" ).toInt(),
			_parent_item, getMainWindow(), node.toElement().
						attribute( "id" ).toInt() );
				c->hide();
			}
			else
			{
				client * c = client::clientFromID(
						node.toElement().attribute(
							"id" ).toInt() );
				if( c == NULL )
				{
					continue;
				}
				c->parentWidget()->move(
			node.toElement().attribute( "x" ).toInt(),
			node.toElement().attribute( "y" ).toInt() );
				c->m_rasterX = node.toElement().attribute(
								"x" ).toInt();
				c->m_rasterY = node.toElement().attribute(
								"y" ).toInt();
				c->setFixedSize( node.toElement().attribute(
								"w" ).toInt(),
						node.toElement().attribute(
								"h" ).toInt() );

				if( node.toElement().attribute( "visible" ) ==
									"yes" )
				{
					c->show();
				}
				else
				{
					c->hide();
				}
			}
		}
        }
}




void classroomManager::loadGlobalClientConfig( void )
{
	m_view->clear();

	if( !QFileInfo( m_globalClientConfiguration ).exists() &&
		QFileInfo( m_globalClientConfiguration + ".bak" ).exists() )
	{
		QFile( m_globalClientConfiguration + ".bak" ).copy(
						m_globalClientConfiguration );
	}

	// read the XML file and create DOM tree
	QFile cfg_file( m_globalClientConfiguration );
	if( !cfg_file.open( QIODevice::ReadOnly ) )
	{
		if( splashScreen != NULL )
		{
			splashScreen->close();
		}
		messageBox::information( tr( "No configuration-file found" ),
					tr( "Could not open configuration "
						"file %1.\nYou will have to "
						"add at least one classroom "
						"and computers using the "
						"classroom-manager which "
						"you'll find inside the "
						"program in the sidebar on the "
						"left side."
					).arg( m_globalClientConfiguration ) );
		return;
	}

	if( !m_domTree.setContent( &cfg_file ) )
	{
		if( splashScreen != NULL )
		{
			splashScreen->close();
		}
		messageBox::information( tr( "Error in configuration-file" ),
					tr( "Error while parsing configuration-"
						"file %1.\nPlease edit it. "
						"Otherwise you should delete "
						"this file and have to add all "
						"classrooms and computers "
						"again."
					).arg( m_globalClientConfiguration ),
					QPixmap( ":/resources/error.png" ) );
		cfg_file.close();
		return;
	}
	cfg_file.close();


	// get the head information from the DOM
	QDomElement root = m_domTree.documentElement();
	QDomNode node = root.firstChild();

	// create the tree view out of the DOM
	node = root.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() && node.nodeName() == "body" )
		{
			loadTree( NULL, node.toElement(), TRUE );
			break;
		}
		node = node.nextSibling();
	}
}




void classroomManager::loadPersonalConfig( void )
{
	if( !QFileInfo( m_personalConfiguration ).exists() &&
		QFileInfo( m_personalConfiguration + ".bak" ).exists() )
	{
		QFile( m_personalConfiguration + ".bak" ).copy(
						m_personalConfiguration );
	}

	// read the XML file and create DOM tree
	QFile cfg_file( m_personalConfiguration );
	if( !cfg_file.open( QIODevice::ReadOnly ) )
	{
		return;
	}

	if( !m_domTree.setContent( &cfg_file ) )
	{
		if( splashScreen != NULL )
		{
			splashScreen->close();
		}
		messageBox::information( tr( "Error in configuration-file" ),
					tr( "Error while parsing configuration-"
						"file %1.\nPlease edit it. "
						"Otherwise you should delete "
						"this file."
					).arg( m_personalConfiguration ),
					QPixmap( ":/resources/error.png" ) );
		cfg_file.close();
		return;
	}
	cfg_file.close();


	// get the head information from the DOM
	QDomElement root = m_domTree.documentElement();
	QDomNode node = root.firstChild();

	while( !node.isNull() )
	{
		if( node.isElement() && node.nodeName() == "head" )
		{
			getHeaderInformation( node.toElement() );
			break;
		}
		node = node.nextSibling();
	}

	// create the tree view out of the DOM
	node = root.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() && node.nodeName() == "body" )
		{
			loadTree( NULL, node.toElement(), FALSE );
			break;
		}
		node = node.nextSibling();
	}
}




void classroomManager::createActionMenu( QMenu * _m )
{
/*for( int i = 0; i < m_view->topLevelItemCount(); ++i )
	{
		QTreeWidgetItem * lvi = m_view->topLevelItem( i );
		if( lvi->childCount() )
		{
			createActionMenu( lvi, _m );
		}
	}*/
}




void classroomManager::createActionMenu( QTreeWidgetItem * _p, QMenu * _m )
{
/*	QMenu * root = _m->addMenu( _p->text( 0 ) );

	classRoomItem * l = NULL;

	for( int i = 0; i < _p->childCount(); ++i )
	{
		QTreeWidgetItem * lvi = _p->child( i );
		if( lvi->childCount() )
		{
			createActionMenu( lvi, root );
		}
		else if( ( l = dynamic_cast<classRoomItem *>( lvi ) ) != NULL )
		{
			l->getClient()->createActionMenu(
				root->addMenu( l->getClient()->fullName() ) );
		}
	}*/
}




void classroomManager::updateClients( void )
{
	QVector<client *> vc = visibleClients();

	foreach( client * cl, vc )
	{
		// update current client
		cl->update();
	}

	QTimer::singleShot( m_clientUpdateInterval * 1000, this,
						SLOT( updateClients() ) );
}







inline void classroomManager::cmdToVisibleClients( client::clientCmds _cmd,
						const QString & _u_data )
{
	QVector<client *> vc = visibleClients();
	foreach( client * cl, vc )
	{
		cl->processCmd( _cmd, _u_data );
	}
}





void classroomManager::changeGlobalClientMode( int _mode )
{
	client::modes new_mode = static_cast<client::modes>( _mode );
	if( new_mode != m_globalClientMode ||
					new_mode == client::Mode_Overview )
	{
		m_globalClientMode = new_mode;
		isdConnection * conn = getMainWindow()->localISD();
		QVector<client *> vc = visibleClients();

		foreach( client * cl, vc )
		{
			cl->changeMode( m_globalClientMode, conn );
		}
	}
}




void classroomManager::powerOnClients( void )
{
/*	progressInformation pi(
		tr( "Please wait, while the clients are being turned on." ) );*/
	cmdToVisibleClients( client::Cmd_PowerOn );
}




void classroomManager::multiLogon( void )
{
	multiLogonDialog mld( this );
	if( mld.exec() == QDialog::Accepted &&
		!mld.userName().isEmpty() && !mld.password().isEmpty() )
	{
		cmdToVisibleClients( client::Cmd_LogonUser,
				mld.userName() + "*" + mld.password() +
							"*" + mld.domain() );
	}
}




void classroomManager::powerDownClients( void )
{
	if( QMessageBox::question( window(), tr( "Power down computers" ),
					tr( "Are you sure want to power down "
						"all visible computers?" ),
					QMessageBox::Yes, QMessageBox::No ) ==
							QMessageBox::Yes )
	{
		/*progressInformation pi( tr( "Please wait, while the computers "
						"are being powered down." ) );*/
		cmdToVisibleClients( client::Cmd_PowerDown );
	}
}




void classroomManager::logoutUser( void )
{
	if( QMessageBox::question( window(), tr( "Logout user" ),
			tr( "Are you sure want logout all users on all "
							"visible computers ?" ),
					QMessageBox::Yes, QMessageBox::No ) ==
							QMessageBox::Yes )
	{
		cmdToVisibleClients( client::Cmd_LogoutUser );
	}
}




void classroomManager::execCmds( void )
{
	QString cmds;

	cmdInputDialog cmd_input_dialog( cmds, this );
	if( cmd_input_dialog.exec() == QDialog::Accepted && cmds != "" )
	{
		cmdToVisibleClients( client::Cmd_ExecCmds, cmds );
	}
}




void classroomManager::sendMessage( void )
{
	QString msg;

	textMessageDialog tmd( msg, this );
	if( tmd.exec() == QDialog::Accepted && msg != "" )
	{
		/*progressInformation pi(
			tr( "Please wait, while the message is being sent." ) );*/
		cmdToVisibleClients( client::Cmd_SendTextMessage, msg );
	}
}




const int decor_w = 8;
const int decor_h = 34;


void classroomManager::adjustWindows( void )
{
	QVector<client *> vc = visibleClients();
	if( vc.size() )
	{
		const int avail_w = getMainWindow()->workspace()->width();
		const int avail_h = getMainWindow()->workspace()->height();
		float cw = vc[0]->width() + decor_w;// add width of decoration
		float ch = vc[0]->height() + decor_h;// add height of titlebar

		// later we divide by cw, so assume standard-value if zero
		if( static_cast<int>( cw ) == 0 )
		{
			cw = 256.0f;
		}
		// later we divide by ch, so assume standard-value if zero
		if( static_cast<int>( ch ) == 0 )
		{
			ch = 192.0f;
		}
		int x_offset = vc[0]->parentWidget()->pos().x();
		int y_offset = vc[0]->parentWidget()->pos().y();

		foreach( client * cl, vc )
		{
			if( cl->parentWidget()->pos().x() < x_offset )
			{
				x_offset = cl->parentWidget()->pos().x();
			}
			if( cl->parentWidget()->pos().y() < y_offset )
			{
				y_offset = cl->parentWidget()->pos().y();
			}
		}

		float max_rx = 0.0;
		float max_ry = 0.0;
		foreach( client * cl, vc )
		{
			cl->m_rasterX = roundCorrect( (
				cl->parentWidget()->pos().x()-x_offset ) / cw );
			cl->m_rasterY = roundCorrect( (
				cl->parentWidget()->pos().y()-y_offset ) / ch );
			if( cl->m_rasterX > max_rx )
			{
				max_rx = cl->m_rasterX;
			}
			if( cl->m_rasterY > max_ry )
			{
				max_ry = cl->m_rasterY;
			}
		}
		++max_rx;
		++max_ry;

		// now we have length of col and length of row and can
		// calculate a width and a height (independent from each other)

		// divide available width by max length of rows
		int nw = static_cast<int>( floor( avail_w / max_rx ) );
		// calculate according height
		int nh = ( nw - decor_w ) * 3 / 4 + decor_h;
		// is this height fit max_ry times into available height?
		if( nh * max_ry >= avail_h )
		{
			// no then divide available height by max length of cols
			nh = static_cast<int>( floor( avail_h / max_ry ) );
			// and calculate according width
			nw = ( nh - decor_h ) * 4 / 3 + decor_w;
		}

		foreach( client * cl, vc )
		{
			cl->setFixedSize( nw - decor_w, nh - decor_h );
			cl->parentWidget()->move(
				static_cast<int>( cl->m_rasterX * nw ),
				static_cast<int>( cl->m_rasterY * nh ) );
		}
	}
}




bool clientIPSort( const client * _1, const client * _2 )
{
	return( _1->remoteIP() < _2->remoteIP() );
}




void classroomManager::arrangeWindows( void )
{
	QVector<client *> vc = visibleClients();
	if( vc.size() )
	{
		const int avail_w = getMainWindow()->workspace()->width();
		const int avail_h = getMainWindow()->workspace()->height();
		const int w = avail_w;
		const int h = avail_h;
		const float s = sqrt( vc.size() * h / (float) w );
		const int win_per_line = (int) round( vc.size() / round( s ) );
		const int win_per_row = (int) round( s );
		const int ww = avail_w / win_per_line;
		const int wh = avail_h / win_per_row;

		qSort( vc.begin(), vc.end(), clientIPSort );

		int i = 0;
		foreach( client * cl, vc )
		{
			cl->parentWidget()->move( ( i % win_per_line ) *
							( ww + decor_w ),
						( i / win_per_line ) *
							( wh + decor_h ) );
			cl->setFixedSize( ww, wh );
			++i;
		}
		adjustWindows();
	}
}




void classroomManager::resizeClients( const int _new_width )
{
	QVector<client *> vc = visibleClients();

	if( vc.size() )
	{
		const int _new_height = _new_width * 3 / 4;
		float cw = vc[0]->width() + decor_w;	// add width of
							// decoration
		float ch = vc[0]->height() + decor_h;	// add height of
							// titlebar
		// later we divide by cw, so assume standard-value if zero
		if( static_cast<int>( cw ) == 0 )
		{
			cw = 256.0f;
		}
		// later we divide by ch, so assume standard-value if zero
		if( static_cast<int>( ch  )== 0 )
		{
			ch = 192.0f;
		}

		int x_offset = vc[0]->parentWidget()->pos().x();
		int y_offset = vc[0]->parentWidget()->pos().y();

		foreach( client * cl, vc )
		{
			if( cl->parentWidget()->pos().x() < x_offset )
			{
				x_offset = cl->parentWidget()->pos().x();
			}
			if( cl->parentWidget()->pos().y() < y_offset )
			{
				y_offset = cl->parentWidget()->pos().y();
			}
		}

		foreach( client * cl, vc )
		{
			cl->setFixedSize( _new_width, _new_height );
			const int xp = static_cast<int>( (
				cl->parentWidget()->pos().x() - x_offset ) /
				cw * ( _new_width + decor_w ) ) + x_offset;
			const int yp = static_cast<int>( (
				cl->parentWidget()->pos().y() - y_offset ) /
				ch * ( _new_height + decor_h ) ) + y_offset;
			cl->parentWidget()->move( xp, yp );
		}
	}
}




void classroomManager::increaseClientSize( void )
{
	QVector<client *> vc = visibleClients();

	if( vc.size() )
	{
		const int cw = vc[0]->width();
		int i = 0;
		// seek to first width which is greater than current
		// client-width
		while( widths[i] > 0 && cw >= widths[i] )
		{
			++i;
		}

		if( widths[i] > 0 )
		{
			resizeClients( widths[i] );
		}
	}
}




void classroomManager::decreaseClientSize( void )
{
	QVector<client *> vc = visibleClients();

	if( vc.size() )
	{
		const int cw = vc[0]->width();
		int i = 0;
		// seek to last width
		while( widths[i] > 0 )
		{
			++i;
		}
		--i;
		// seek to first width which is smaller than current
		// client-width
		while( i > 0 && cw <= widths[i] )
		{
			--i;
		}

		if( i >= 0 && widths[i] > 0 )
		{
			resizeClients( widths[i] );
		}
	}
}




void classroomManager::updateIntervalChanged( int _value )
{
	m_clientUpdateInterval = _value;
}




void classroomManager::itemDoubleClicked( QTreeWidgetItem * _i, int )
{
	classRoomItem * cri = dynamic_cast<classRoomItem *>( _i );

	if( cri != NULL )
	{
		if( cri->getClient()->isVisible() )
		{
			cri->getClient()->hide();
		}
		else
		{
			cri->getClient()->show();
		}
	}
}




void classroomManager::contextMenuRequest( const QPoint & _pos )
{
	QMenu * context_menu = new QMenu( this );

	QTreeWidgetItem * i = m_view->itemAt( _pos );
	m_view->setCurrentItem( i );
	classRoomItem * cri = dynamic_cast<classRoomItem *>( i );
	classRoom * cr = dynamic_cast<classRoom *>( i );

	if( cri != NULL )
	{
		context_menu->addAction(
				QPixmap( ":/resources/client_show.png" ),
						tr( "Show/hide" ),
					this, SLOT( showHideClient() ) );

		context_menu->addAction(
				QPixmap( ":/resources/client_settings.png" ),
						tr( "Edit settings" ), this,
					SLOT( editClientSettings() ) );

		context_menu->addAction(
				QPixmap( ":/resources/client_remove.png" ),
						tr( "Remove" ), this,
						SLOT( removeClient() ) );

		context_menu->addSeparator();

		QMenu * single_clients_submenu = context_menu->addMenu(
					QPixmap( ":/resources/client.png" ),
							tr( "Actions" ) );

		cri->getClient()->createActionMenu( single_clients_submenu );

		context_menu->addSeparator();
	}
	else if( cr != NULL )
	{
		context_menu->addAction(
				QPixmap( ":/resources/classroom_show.png" ),
				tr( "Show all computers in classroom" ),
				this, SLOT( showSelectedClassRooms() ) );

		context_menu->addAction(
				QPixmap( ":/resources/classroom_closed.png" ),
				tr( "Hide all computers in classroom" ),
				this, SLOT( hideSelectedClassRooms() ) );

		context_menu->addAction(
				QPixmap( ":/resources/client_settings.png" ),
				tr( "Edit name" ),
				this, SLOT( editClassRoomName() ) );

		context_menu->addAction(
				QPixmap( ":/resources/classroom_remove.png" ),
				tr("Remove classroom" ),
				this, SLOT( removeClassRoom() ) );

		context_menu->addSeparator();

		QMenu * actions_for_classroom_submenu = context_menu->addMenu(
				QPixmap( ":/resources/classroom_opened.png" ),
				tr( "Actions for %1" ).arg( cr->text( 0 ) ) );
		cr->createActionMenu( actions_for_classroom_submenu, FALSE );

		context_menu->addSeparator();

	}
	else if( m_classRooms.size() )
	{ 
		QMenu * actions_for_classroom_submenu = context_menu->addMenu(
				QPixmap( ":/resources/classroom_closed.png" ),
				tr( "Action for whole classroom" ) );
		for( QVector<classRoom *>::iterator it = m_classRooms.begin();
						it != m_classRooms.end(); ++it )
		{
			( *it )->createActionMenu(
						actions_for_classroom_submenu );
		}

		context_menu->addSeparator();
	}

	context_menu->addAction( QPixmap( ":/resources/client_add.png" ),
					tr( "Add computer" ),
					this, SLOT( addClient() ) );

	context_menu->addAction( QPixmap( ":/resources/classroom_add.png" ),
					tr( "Add classroom" ),
					this, SLOT( addClassRoom() ) );

	context_menu->exec( QCursor::pos() );

	delete context_menu;
}




QVector<classRoomItem *> classroomManager::selectedItems( void )
{
	QVector<classRoomItem *> vc;

	for( int i = 0; i < m_view->topLevelItemCount(); ++i )
	{
		getSelectedItems( m_view->topLevelItem( i ), vc );
	}

	return( vc );
}




void classroomManager::getSelectedItems( QTreeWidgetItem * _p,
					QVector<classRoomItem *> & _vc,
								bool _add_all )
{
	for( int i = 0; i < _p->childCount(); ++i )
	{
		QTreeWidgetItem * lvi = _p->child( i );
		if( lvi->childCount() )
		{
			getSelectedItems( lvi, _vc,
						m_view->isItemSelected( lvi ) );
		}
		else if( _add_all ||
			( dynamic_cast<classRoomItem *>( lvi ) != NULL &&
					m_view->isItemSelected( lvi ) ) )
		{
			_vc.push_back( dynamic_cast<classRoomItem *>( lvi ) );
		}
	}
}




// slots for client-actions in context-menu
void classroomManager::showHideClient( void )
{
	QVector<classRoomItem *> si = selectedItems();

	if( si.size() > 0 )
	{
		bool all_shown = TRUE;
		foreach( classRoomItem * cri, si )
		{
			if( cri->getClient()->isVisible() == FALSE )
			{
				all_shown = FALSE;
				break;
			}
		}
		foreach( classRoomItem * cri, si )
		{
			cri->getClient()->setVisible( all_shown );
		}
	}
}




void classroomManager::editClientSettings( void )
{
	QVector<classRoomItem *> si = selectedItems();

	if( si.size() > 0 )
	{
		foreach( classRoomItem * cri, si )
		{
			clientSettingsDialog settings_dlg( cri->getClient(),
					getMainWindow(),
						cri->parent()->text( 0 ) );
			settings_dlg.exec();
		}
		saveGlobalClientConfig();
		savePersonalConfig();
	}
}




void classroomManager::removeClient( void )
{
	QVector<classRoomItem *> si = selectedItems();

	if( si.size() > 0 )
	{
		foreach( classRoomItem * cri, si )
		{
			cri->getClient()->hide();
			m_clientsToRemove.push_back( cri->getClient() );
			m_view->setItemHidden( cri, TRUE );
		}
	}
}




void classroomManager::setStateOfClassRoom( classRoom * _cr, bool _shown )
{
	if( _shown )
	{
		_cr->setMenuItemIcon( QIcon( ":/resources/greenled.png" ) );
	}
	else
	{
		_cr->setMenuItemIcon( QIcon() );
	}

	// If all clients are shown, we hide them all. Otherwise we show all.
	for( int i = 0; i < _cr->childCount(); ++i )
	{
		QTreeWidgetItem * cri = _cr->child( i );
		if( dynamic_cast<classRoomItem *>( cri ) != NULL )
		{
			dynamic_cast<classRoomItem *>( cri )->getClient()->
							setVisible( _shown );
		}
	}
}




QAction * classroomManager::addClassRoomToQuickSwitchMenu( classRoom * _cr )
{
	QAction * a = new QAction( _cr->text( 0 ), m_quickSwitchMenu );
	connect( a, SIGNAL( triggered( bool ) ), _cr,
						SLOT( switchToClassRoom() ) );
	m_quickSwitchMenu->insertAction( m_qsmClassRoomSeparator, a );
	return( a );
}




void classroomManager::showSelectedClassRooms( void )
{
	foreach( classRoom * cr, m_classRooms )
	{
		if( m_view->isItemSelected( cr ) && cr->childCount() )
		{
			setStateOfClassRoom( cr, TRUE );
		}
	}
}




void classroomManager::hideSelectedClassRooms( void )
{
	foreach( classRoom * cr, m_classRooms )
	{
		if( m_view->isItemSelected( cr ) && ( cr )->childCount() )
		{
			setStateOfClassRoom( cr, FALSE );
		}
	}
}




void classroomManager::hideAllClassRooms( void )
{
	foreach( classRoom * cr, m_classRooms )
	{
		setStateOfClassRoom( cr, FALSE );
	}
}




void classroomManager::editClassRoomName( void )
{
	foreach( classRoom * cr, m_classRooms )
	{
		if( m_view->isItemSelected( cr ) == FALSE )
		{
			continue;
		}
		QString classroom_name = cr->text( 0 );

		bool ok;
		classroom_name = QInputDialog::getText( this,
			tr( "New name for classroom" ),
			tr( "Please enter a new name for classroom \"%1\"." ).
				arg( classroom_name ), QLineEdit::Normal,
							classroom_name, &ok );
		if( ok && !classroom_name.isEmpty() )
		{
			cr->setText( 0, classroom_name );
		}
		saveGlobalClientConfig();
		savePersonalConfig();
	}
}




void classroomManager::removeClassRoom( void )
{
	foreach( classRoom * cr, m_classRooms )
	{
		if( m_view->isItemSelected( cr ) == FALSE )
		{
			continue;
		}
		if( QMessageBox::question( window(), tr( "Remove classroom" ),
			tr( "Are you sure want to remove classroom \"%1\"?\n"
				"All computers in it will be removed as well!" ).
							arg( cr->text( 0 ) ),
							QMessageBox::Yes,
							QMessageBox::No ) ==
					QMessageBox::No )
		{
			continue;
		}

		m_view->setItemHidden( cr, TRUE );

		QVector<client *> vc;
		getVisibleClients( cr, vc );
		foreach( client * cl, vc )
		{
			cl->hide();
			m_clientsToRemove.push_back( cl );
		}
		m_classRoomsToRemove.push_back( cr );
	}
}




// slots for general actions in context-menu
void classroomManager::addClient( void )
{
	if( m_classRooms.size() == 0 )
	{
		if( QMessageBox::question( window(), tr( "Missing classroom" ),
						tr( "Before adding computers "
							"you have to "
							"create at least one "
							"classroom.\nDo you "
							"want to create a new "
							"classrom now?" ),
						QMessageBox::Yes,
						QMessageBox::No ) ==
							QMessageBox::No )
		{
			return;
		}
		addClassRoom();
		if( m_classRooms.size() == 0 )
		{
			return;
		}
	}

	QString classroom_name = "";

	foreach( classRoom * cr, m_classRooms )
	{
		if( m_view->isItemSelected( cr ) )
		{
			classroom_name = ( cr )->text( 0 );
			break;
		}
	}

	clientSettingsDialog settings_dlg( NULL, getMainWindow(),
							classroom_name );
	settings_dlg.setWindowTitle( tr( "Add computer" ) );
	settings_dlg.exec();
	saveGlobalClientConfig();
	savePersonalConfig();
}




void classroomManager::addClassRoom( void )
{
	bool ok;
	QString classroom_name = QInputDialog::getText( this,
			tr( "New classroom" ),
			tr( "Please enter the name of the classroom you "
							"want to create." ),
			QLineEdit::Normal, tr( "New classroom" ), &ok );
	if( ok && !classroom_name.isEmpty() )
	{
		classRoom * sel_cr = NULL;
		foreach( classRoom * cr, m_classRooms )
		{
			if( m_view->isItemSelected( cr ) )
			{
				sel_cr = cr;
				break;
			}
		}
		if( sel_cr != NULL )
		{
			m_classRooms.push_back( new classRoom( classroom_name,
							this, sel_cr ) );
		}
		else
		{
			m_classRooms.push_back( new classRoom( classroom_name,
							this, m_view ) );
		}
		saveGlobalClientConfig();
	}
}




void classroomManager::hideTeacherClients( void )
{
	QVector<client *> vc = visibleClients();

	foreach( client * cl, vc )
	{
		if( cl->type() == client::Type_Teacher )
		{
			cl->hide();
		}
	}
}








classRoom::classRoom( const QString & _name,
					classroomManager * _classroom_manager,
						QTreeWidgetItem * _parent ) :
	QTreeWidgetItem( _parent, QStringList( _name ) ),
	m_classroomManager( _classroom_manager ),
	m_qsMenuAction( m_classroomManager->addClassRoomToQuickSwitchMenu(
									this ) )
{
}




classRoom::classRoom( const QString & _name,
					classroomManager * _classroom_manager,
						QTreeWidget * _parent ) :
	QTreeWidgetItem( _parent, QStringList( _name ) ),
	m_classroomManager( _classroom_manager ),
	m_qsMenuAction( m_classroomManager->addClassRoomToQuickSwitchMenu(
									this ) )
{
}




classRoom::~classRoom()
{
	delete m_qsMenuAction;
}




void classRoom::createActionMenu( QMenu * _m, bool _add_sub_menu )
{
	QMenu * this_classroom_submenu = _m;
	QMenu * orig_root = _m;

	if( _add_sub_menu )
	{
		orig_root = this_classroom_submenu;
		this_classroom_submenu = _m->addMenu( text( 0 ) );
	}
	connect( this_classroom_submenu, SIGNAL( triggered( QAction * ) ),
			this, SLOT( processCmdOnAllClients( QAction * ) ) );
	for( int i = client::Cmd_SendTextMessage; i < client::Cmd_CmdCount;
									++i )
	{
		if( i == client::Cmd_ExecCmds )
		{
			this_classroom_submenu =
				this_classroom_submenu->addMenu( QPixmap(
					":/resources/client_settings.png" ),
							tr( "Administation" ) );

			connect( this_classroom_submenu,
					SIGNAL( triggered( QAction * ) ),
				this,
				SLOT( processCmdOnAllClients( QAction * ) ) );
		}
		QAction * cur = this_classroom_submenu->addAction(
				QPixmap( QString( ":/resources/" ) +
						client::s_commands[i].m_icon ),
				client::tr( client::s_commands[i].m_name ) );
		cur->setData( i );

		if( client::s_commands[i].m_insertSep )
		{
			this_classroom_submenu->addSeparator();
		}
	}
}




void classRoom::processCmdOnAllClients( QAction * _action )
{
	QVector<client *> vc;

	// get all visible clients for this classroom
	classroomManager::getVisibleClients( this, vc );

	QString u_data = CONFIRM_NO;

	int cmd = _action->data().toInt();

	// may be we have to do something (e.g. confirm) before executing cmd
	switch( static_cast<client::clientCmds>( cmd ) )
	{
		case client::Cmd_SendTextMessage:
		{
			textMessageDialog tmd( u_data, m_classroomManager );
			if( tmd.exec() != QDialog::Accepted || u_data == "" )
			{
				return;
			}
			break;
		}

		case client::Cmd_LogoutUser:
		{
			if( QMessageBox::question( m_classroomManager->window(),
					classroomManager::tr( "Logout user" ),
				classroomManager::tr( "Are you sure want logout "
							"all users on all "
							"visible computers ?" ),
						QMessageBox::Yes,
						QMessageBox::No ) ==
							QMessageBox::No )
			{
				return;
			}
			break;
		}

		case client::Cmd_ExecCmds:
		{
			cmdInputDialog cmd_input_dialog( u_data, NULL );
			if( cmd_input_dialog.exec() != QDialog::Accepted ||
								u_data == "" )
			{
				return;
			}
			break;
		}

		case client::Cmd_Reboot:
		{
			if( QMessageBox::question( m_classroomManager->window(),
				classroomManager::tr( "Reboot computers" ),
				classroomManager::tr( "Are you sure want to "
					"reboot all visible computers?" ),
					QMessageBox::Yes, QMessageBox::No ) ==
							QMessageBox::No )
			{
				return;
			}
			break;
		}
		case client::Cmd_PowerDown:
		{
			if( QMessageBox::question( m_classroomManager->window(),
				classroomManager::tr( "Power down computers" ),
				classroomManager::tr( "Are you sure want to power "
						"down all visible computers?" ),
					QMessageBox::Yes, QMessageBox::No ) ==
							QMessageBox::No )
			{
				return;
			}
			break;
		}
		default:
			break;
	}

	for( QVector<client *>::iterator it = vc.begin(); it != vc.end(); ++it )
	{
		( *it )->processCmd( static_cast<client::clientCmds>( cmd ),
								u_data );
	}
}





void classRoom::switchToClassRoom( void )
{
	m_classroomManager->hideAllClassRooms();
	m_classroomManager->setStateOfClassRoom( this, TRUE );
}










classRoomItem::classRoomItem( client * _client, QTreeWidgetItem * _parent ) :
	QTreeWidgetItem( _parent, QStringList( _client->name() ) ),
	m_client( _client )
{
	if( s_clientPixmap == NULL )
	{
		s_clientPixmap = new QPixmap( ":/resources/client_hidden.png" );
	}

	if( s_clientObservedPixmap == NULL )
	{
		s_clientObservedPixmap = new QPixmap(
					":/resources/client_visible.png" );
	}

	setVisible( FALSE );
	setText( 1, m_client->remoteIP() );
}




classRoomItem::~classRoomItem()
{
	m_client->m_classRoomItem = NULL;
}




void classRoomItem::setVisible( const bool _obs )
{
	if( _obs == FALSE )
	{
		setIcon( 0, *s_clientPixmap );
	}
	else
	{
		setIcon( 0, *s_clientObservedPixmap );
	}
}




#include "classroom_manager.moc"

