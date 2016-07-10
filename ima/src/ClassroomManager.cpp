/*
 * ClassroomManager.cpp - implementation of classroom-manager
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

#include <italcconfig.h>
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
#include <QtGui/QPushButton>
#include <QtGui/QSplashScreen>
#include <QtGui/QSplitter>
#include <QtGui/QPainter>


#include "MainWindow.h"
#include "ClassroomManager.h"
#include "Client.h"
#include "Dialogs.h"
#include "RunCommandsDialog.h"
#include "ItalcConfiguration.h"
#include "LocalSystem.h"
#include "ToolButton.h"
#include "DecoratedMessageBox.h"

#define DEFAULT_WINDOW_WIDTH	1005
#define DEFAULT_WINDOW_HEIGHT	700


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



ClassroomManager::ClassroomManager( MainWindow * _main_window,
							QWidget * _parent ) :
	SideBarWidget( QPixmap( ":/resources/classroom_manager.png" ),
			tr( "Classroom-Manager" ),
			tr( "Use this workspace to manage your computers and "
				"classrooms in an easy way." ),
			_main_window, _parent ),
	m_personalConfiguration( LocalSystem::Path::expand( ItalcCore::config->personalConfigurationPath() ) ),
	m_globalClientConfiguration( LocalSystem::Path::expand( ItalcCore::config->globalConfigurationPath() ) ),
	m_quickSwitchMenu( new QMenu( this ) ),
	m_qsmClassRoomSeparator( m_quickSwitchMenu->addSeparator() ),
	m_globalClientMode( Client::Mode_Overview ),
	m_clientUpdateInterval( 500 ),
	m_autoArranged( false )
{
	// some code called out of this function relies on m_classroomManager
	// which actually is assigned after this function returns
	_main_window->m_classroomManager = this;

	QVBoxLayout * l = new QVBoxLayout( contentParent() );

	m_view = new classTreeWidget( contentParent() );
	l->addWidget( m_view );
	m_view->setWhatsThis( tr( "This is where computers and classrooms are "
					"managed. You can add computers or "
					"classrooms by clicking right "
					"in this list." ) );

	QStringList columns;
	columns << tr( "Classrooms/computers" ) << tr( "IP-address" ) << tr( "Usernames" );
	m_view->setHeaderLabels( columns );
	m_view->hideColumn( 2 );
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

	m_showUsernameCheckBox = new QCheckBox( tr( "Show usernames" ), contentParent() );
	m_showUsernameCheckBox->setFont( f );
	l->addWidget( m_showUsernameCheckBox );
	connect( m_showUsernameCheckBox, SIGNAL( stateChanged( int ) ),
			this, SLOT( showUserColumn( int ) ) );

	l->addSpacing( 5 );
	QLabel * help_txt = new QLabel(
		tr( "Use the context-menu (right mouse-button) to add/remove "
			"computers and/or classrooms." ), contentParent() );
	l->addWidget( help_txt );
	help_txt->setWordWrap( TRUE );
	help_txt->setFont( f );

	l->addSpacing( 16 );

	m_exportToFileBtn = new QPushButton(
				QPixmap( ":/resources/filesave.png" ),
						tr( "Export to text-file" ),
							contentParent() );
	l->addWidget( m_exportToFileBtn );
	connect( m_exportToFileBtn, SIGNAL( clicked() ),
			this, SLOT( clickedExportToFile() ) );
	m_exportToFileBtn->setWhatsThis( tr( "Use this button for exporting "
				"this list of computers and usernames into "
				"a text-file. You can use this file "
				"later for collecting files "
				"after an exam has finished. "
				"This is sometimes neccessary, "
				"because some users might have "
				"finished and logged out "
				"earlier and so you cannot "
				"collect their files at the "
				"end of the exam." ) );

	setupMenus();

	loadGlobalClientConfig();
	loadPersonalConfig();

	show();
}




void ClassroomManager::setupMenus()
{
	QAction * act;
	QAction * separator = new QAction( this );
	separator->setSeparator( TRUE );

	/*** quick workspace menu ***/

	m_quickSwitchMenu->addAction( tr( "Hide teacher computers" ),
					this, SLOT( hideTeacherClients() ) );

/*	m_quickSwitchMenu->addSeparator();

	m_quickSwitchMenu->addAction( QPixmap( ":/resources/adjust_size.png" ),
				tr( "Adjust windows and their size" ),
						this, SLOT( adjustWindows() ) );

	m_quickSwitchMenu->addAction(
				QPixmap( ":/resources/auto_arrange.png" ),
				tr( "Auto re-arrange windows" ),
					this, SLOT( arrangeWindows() ) );*/

	/*** actions for single client in context Menu ***/

	m_classRoomItemActionGroup = new QActionGroup( this );

	act = m_classRoomItemActionGroup->addAction( 
			QPixmap( ":/resources/client_show.png" ),
			tr( "Show/hide" ) );
	connect( act, SIGNAL( triggered() ), this, SLOT( showHideClient() ) );

	act = m_classRoomItemActionGroup->addAction( 
			QPixmap( ":/resources/config.png" ),
			tr( "Edit settings" ) );
	connect( act, SIGNAL( triggered() ), this, SLOT( editClientSettings() ) );

	act = m_classRoomItemActionGroup->addAction(
			QPixmap( ":/resources/client_remove.png" ),
			tr( "Remove" ) );
	connect( act, SIGNAL( triggered() ), this, SLOT( removeClient() ) );

	m_classRoomItemActionGroup->addAction( "" )->setSeparator( TRUE );

	/*** actions for single classroom in context Menu ***/

	m_classRoomActionGroup = new QActionGroup( this );

	act = m_classRoomActionGroup->addAction(
			QPixmap( ":/resources/classroom_show.png" ),
			tr( "Show all computers in classroom" ) );
	connect( act, SIGNAL( triggered() ), this, SLOT( showSelectedClassRooms() ) );

	act = m_classRoomActionGroup->addAction(
			QPixmap( ":/resources/classroom_closed.png" ),
			tr( "Hide all computers in classroom" ) );
	connect( act, SIGNAL( triggered() ), this, SLOT( hideSelectedClassRooms() ) );

	act = m_classRoomActionGroup->addAction(
			QPixmap( ":/resources/no_user.png" ),
			tr( "Hide teacher computers" ) );
	connect( act, SIGNAL( triggered() ), this, SLOT( hideTeacherClients() ) );

	act = m_classRoomActionGroup->addAction(
			QPixmap( ":/resources/config.png" ),
			tr( "Edit name" ) );
	connect( act, SIGNAL( triggered() ), this, SLOT( editClassRoomName() ) );

	act = m_classRoomActionGroup->addAction(
			QPixmap( ":/resources/classroom_remove.png" ),
			tr("Remove classroom" ) );
	connect( act, SIGNAL( triggered() ), this, SLOT( removeClassRoom() ) );

	m_classRoomActionGroup->addAction( "" )->setSeparator( TRUE );

	/*** common actions in context menu ***/

	m_contextActionGroup = new QActionGroup( this );

	act = m_contextActionGroup->addAction(
			QPixmap( ":/resources/client_add.png" ),
			tr( "Add computer" ) );
	connect( act, SIGNAL( triggered() ), this, SLOT( addClient() ) );

	act = m_contextActionGroup->addAction(
			QPixmap( ":/resources/classroom_add.png" ),
			tr( "Add classroom" ) );
	connect( act, SIGNAL( triggered() ), this, SLOT( addClassRoom() ) );

	/*** Default client Menu ***/

	m_clientMenu = clientMenu::createDefault( this );
	
}




ClassroomManager::~ClassroomManager()
{
}




void ClassroomManager::doCleanupWork( void )
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
}




void ClassroomManager::saveGlobalClientConfig( void )
{
	QDomDocument doc( "italc-config-file" );

	QDomElement italc_config = doc.createElement( "globalclientconfig" );
	italc_config.setAttribute( "version", ITALC_VERSION );
	doc.appendChild( italc_config );

	QDomElement root = doc.createElement( "body" );
	italc_config.appendChild( root );

	for( int i = 0; i < m_view->topLevelItemCount(); ++i )
	{
		saveSettingsOfChildren( doc, root, m_view->topLevelItem( i ),
									TRUE );
	}

	QString xml = "<?xml version=\"1.0\"?>\n" + doc.toString( 2 );
/*	if( MainWindow::ensureConfigPathExists() == FALSE )
	{
		qFatal( QString( "Could not read/write or create directory %1!"
					"For running iTALC, make sure you have "
					"write-access to your home-directory "
					"and to %1 (if already existing)."
					).arg( ITALC_CONFIG_PATH
						).toUtf8().constData() );
	}*/

	QFile( m_globalClientConfiguration + ".bak" ).remove();
	QFile( m_globalClientConfiguration ).copy( m_globalClientConfiguration +
									".bak" );
	QFile outfile( m_globalClientConfiguration );
	outfile.open( QFile::WriteOnly | QFile::Truncate );

	outfile.write( xml.toUtf8() );
	outfile.close();
}




void ClassroomManager::savePersonalConfig( void )
{
	QDomDocument doc( "italc-config-file" );

	QDomElement italc_config = doc.createElement( "personalconfig" );
	italc_config.setAttribute( "version", ITALC_VERSION );
	doc.appendChild( italc_config );

	QDomElement head = doc.createElement( "head" );
	italc_config.appendChild( head );

	QDomElement globalsettings = doc.createElement( "globalsettings" );
	globalsettings.setAttribute( "client-update-interval",
						m_clientUpdateInterval );
	globalsettings.setAttribute( "win-width", mainWindow()->width() );
	globalsettings.setAttribute( "win-height", mainWindow()->height() );
	globalsettings.setAttribute( "win-x", mainWindow()->x() );
	globalsettings.setAttribute( "win-y", mainWindow()->y() );	
	globalsettings.setAttribute( "ismaximized",
					mainWindow()->isMaximized() );
	globalsettings.setAttribute( "opened-tab",
				mainWindow()->sideBar()->activeTab() );

	globalsettings.setAttribute( "wincfg", QString(
				mainWindow()->saveState().toBase64() ) );

	globalsettings.setAttribute( "defaultdomain", __default_domain );
	globalsettings.setAttribute( "role", ItalcCore::role );
	globalsettings.setAttribute( "notooltips",
					ToolButton::toolTipsDisabled() );
	globalsettings.setAttribute( "icononlymode",
					ToolButton::iconOnlyMode() );
	globalsettings.setAttribute( "clientdoubleclickaction",
						m_clientDblClickAction );
	globalsettings.setAttribute( "showUserColumn",
						m_showUsernameCheckBox->isChecked() );
	globalsettings.setAttribute( "autoarranged",
					isAutoArranged() );

	QStringList hidden_buttons;
	foreach( QAction * a, mainWindow()->toolBar()->actions() )
	{
		if( !a->isVisible() )
		{
			hidden_buttons += a->text();
		}
	}
	foreach( QAbstractButton * btn, mainWindow()->sideBar()->tabs() )
	{
		if( !btn->isVisible() )
		{
			hidden_buttons += btn->text();
		}
	}
	globalsettings.setAttribute( "toolbarcfg", hidden_buttons.join( "#" ) );

	head.appendChild( globalsettings );



	QDomElement root = doc.createElement( "body" );
	italc_config.appendChild( root );

	for( int i = 0; i < m_view->topLevelItemCount(); ++i )
	{
		saveSettingsOfChildren( doc, root, m_view->topLevelItem( i ),
									FALSE );
	}

	foreach ( QDomNode node, m_customMenuConfiguration )
	{
		root.appendChild( node.cloneNode() );
	}

	QString xml = "<?xml version=\"1.0\"?>\n" + doc.toString( 2 );
	QFile( m_personalConfiguration + ".bak" ).remove();
	QFile( m_personalConfiguration ).copy( m_personalConfiguration +
								".bak" );
	QFile outfile( m_personalConfiguration );
	outfile.open( QFile::WriteOnly | QFile::Truncate );

	outfile.write( xml.toUtf8() );
	outfile.close();
}




void ClassroomManager::saveSettingsOfChildren( QDomDocument & _doc,
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
				Client * c = dynamic_cast<classRoomItem *>(
							lvi )->getClient();
				QDomElement client_element = _doc.createElement(
								"client" );
				client_element.setAttribute( "id", c->id() );
				if( _is_global_config )
				{
					client_element.setAttribute( "hostname",
								c->hostname() );
					client_element.setAttribute( "name",
								c->nickname() );
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
			"x", QString::number( c->pos().x() ) );
 						client_element.setAttribute(
			"y", QString::number( c->pos().y() ) );
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
QVector<Client *> ClassroomManager::visibleClients( void ) const
{
	QVector<Client *> vc;
	for( int i = 0; i < m_view->topLevelItemCount(); ++i )
	{
		getVisibleClients( m_view->topLevelItem( i ), vc );
	}

	return( vc );
}




QVector<Client *> ClassroomManager::getLoggedInClients( void ) const
{
	QVector<Client *> loggedClients;

	// loop through all clients
	foreach( Client * it, visibleClients() )
	{
		const QString user = it->user();
		if( user != "none" && !user.isEmpty() )
		{
			loggedClients.push_back( it );
		}
	}
	return( loggedClients );
}




void ClassroomManager::getVisibleClients( QTreeWidgetItem * _p,
						QVector<Client *> & _vc )
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
								l->isVisible() )
		{
			_vc.push_back( l->getClient() );
		}
	}
}




void ClassroomManager::getHeaderInformation( const QDomElement & _header )
{
	QDomNode node = _header.firstChild();

	while( !node.isNull() )
	{
		if( node.isElement() && node.nodeName() == "globalsettings" )
		{
			m_clientUpdateInterval = node.toElement().attribute(
					"client-update-interval" ).toInt();
			// convert old settings
			if( m_clientUpdateInterval < 100 )
			{
				if( m_clientUpdateInterval > 0 )
				{
					m_clientUpdateInterval = m_clientUpdateInterval*100;
				}
				else
				{
					m_clientUpdateInterval = 1000;
				}
			}
			if( m_clientUpdateInterval > 10000 )
			{
				m_clientUpdateInterval = 10000;
			}
			if( node.toElement().attribute( "win-width" ) !=
								QString::null &&
				node.toElement().attribute( "win-height" ) !=
								QString::null &&
				node.toElement().attribute( "win-x" ) !=
								QString::null &&
				node.toElement().attribute( "win-y" ) !=
								QString::null )
			{
mainWindow()->resize( node.toElement().attribute( "win-width" ).toInt(),
			node.toElement().attribute("win-height" ).toInt() );

mainWindow()->move( node.toElement().attribute( "win-x" ).toInt(),
				node.toElement().attribute( "win-y" ).toInt() );
			}
			else
			{
				setDefaultWindowsSizeAndPosition();
 			}
			if( node.toElement().attribute( "opened-tab" ) !=
								QString::null )
			{
				mainWindow()->m_openedTabInSideBar =
						node.toElement().attribute(
							"opened-tab" ).toInt();
			}
			if( node.toElement().attribute( "ismaximized" ).
								toInt() > 0 )
			{
	mainWindow()->setWindowState( mainWindow()->windowState() |
							Qt::WindowMaximized );
			}
			if( node.toElement().attribute( "wincfg" ) !=
								QString::null )
			{
				m_winCfg = node.toElement().attribute(
								"wincfg" );
			}
			if( node.toElement().attribute( "toolbarcfg" ) !=
								QString::null )
			{
				m_toolBarCfg = node.toElement().attribute(
								"toolbarcfg" );
			}
			if( node.toElement().attribute( "autoarranged" ).
								toInt() > 0 )
			{
				m_autoArranged = true;
			}

			__default_domain = node.toElement().
						attribute( "defaultdomain" );

			ItalcCore::role = static_cast<ItalcCore::UserRoles>(
				node.toElement().attribute( "role" ).toInt() );
			if( ItalcCore::role <= ItalcCore::RoleNone ||
				ItalcCore::role >= ItalcCore::RoleCount )
			{
				ItalcCore::role = ItalcCore::RoleTeacher;
			}
			ToolButton::setToolTipsDisabled(
				node.toElement().attribute( "notooltips" ).
								toInt() );
			ToolButton::setIconOnlyMode(
				node.toElement().attribute( "icononlymode" ).
								toInt() );
			m_clientDblClickAction = node.toElement().attribute(
					"clientdoubleclickaction" ).toInt();
			m_showUsernameCheckBox->setChecked(
				node.toElement().attribute( "showUserColumn" ).toInt() );
		}
		node = node.nextSibling();
        }
}




void ClassroomManager::loadTree( classRoom * _parent_item,
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
				QDomElement e = node.toElement();
				QString name = e.attribute( "name" );
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
				QDomElement e = node.toElement();
				QString hostname = e.hasAttribute( "hostname" )
					? e.attribute( "hostname" )
					: e.attribute( "localip" );
				QString mac = e.attribute( "mac" );
				QString nickname = e.attribute( "name" );

				// add new client
                                Client * c = new Client( hostname,
						mac,
						nickname,
						(Client::Types)e.attribute(
							"type" ).toInt(),
						_parent_item,
						mainWindow(),
						e.attribute( "id" ).toInt() );
				c->hide();
			}
			else
			{
				QDomElement e = node.toElement();
				Client * c = Client::clientFromID(
						e.attribute( "id" ).toInt() );
				if( c == NULL )
				{
					continue;
				}
				c->move( e.attribute( "x" ).toInt(),
					e.attribute( "y" ).toInt() );
				c->m_rasterX = e.attribute( "x" ).toInt();
				c->m_rasterY = e.attribute( "y" ).toInt();
				c->setFixedSize( e.attribute( "w" ).toInt(),
						e.attribute( "h" ).toInt() );

				if( e.attribute( "visible" ) == "yes" )
				{
					c->show();
				}
				else
				{
					c->hide();
				}
			}
		}
		else if( node.nodeName() == "menu" )
		{
			m_customMenuConfiguration.append( node.cloneNode() );
			loadMenuElement( node.toElement() );
		}
	}
}




void ClassroomManager::loadMenuElement( QDomElement _e )
{
	if ( _e.hasAttribute( "hide" ) )
	{
		foreach( QAction * act, m_clientMenu->actions() )
		{
			if ( act->text() == _e.attribute( "hide" ) )
			{
				act->setVisible( FALSE );
			}
		}
	}
	else
	{
		QString name = _e.attribute( "remote-cmd",
				_e.attribute( "local-cmd" ) );
		QString icon = _e.attribute( "icon", ":resources/run.png" );
		QString before = _e.attribute( "before" );

		if ( name.isEmpty() )
		{
			return;
		}

		ClientAction::Type type = _e.hasAttribute( "remote-cmd" ) ?
			ClientAction::RemoteScript : ClientAction::LocalScript;

		QAction * act = new ClientAction( type,
				QIcon( icon ), name, m_clientMenu );

		QString cmd;
		for( QDomNode n = _e.firstChild(); ! n.isNull();
			n = n.nextSibling() )
		{
			if ( n.isCharacterData() )
			{
				cmd.append( n.toCharacterData().data() );
			}
		}
		act->setData( cmd );

		QAction * before_act = 0;
		if ( ! before.isEmpty() )
		{
			foreach( QAction * a, m_clientMenu->actions() )
			{
				if ( a->text() == before )
				{
					before_act = a;
					break;
				}
			}
		}

		/* equal to addAction if before_act = 0 */
		m_clientMenu->insertAction( before_act, act );
	}
}




void ClassroomManager::loadGlobalClientConfig( void )
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
		DecoratedMessageBox::information(
					tr( "No configuration-file found" ),
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

	QDomDocument domTree;

	if( !domTree.setContent( &cfg_file ) )
	{
		if( splashScreen != NULL )
		{
			splashScreen->close();
		}
		DecoratedMessageBox::information(
					tr( "Error in configuration-file" ),
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
	QDomElement root = domTree.documentElement();
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




void ClassroomManager::setDefaultWindowsSizeAndPosition( void )
{
	mainWindow()->resize( DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT );
	mainWindow()->move( QPoint( 0, 0 ) );	
}




void ClassroomManager::loadPersonalConfig( void )
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
		setDefaultWindowsSizeAndPosition();
		return;
	}

	QDomDocument domTree;

	if( !domTree.setContent( &cfg_file ) )
	{
		if( splashScreen != NULL )
		{
			splashScreen->close();
		}
		DecoratedMessageBox::information(
					tr( "Error in configuration-file" ),
					tr( "Error while parsing configuration-"
						"file %1.\nPlease edit it. "
						"Otherwise you should delete "
						"this file."
					).arg( m_personalConfiguration ),
					QPixmap( ":/resources/error.png" ) );
		cfg_file.close();
		setDefaultWindowsSizeAndPosition();
		return;
	}
	cfg_file.close();


	// get the head information from the DOM
	QDomElement root = domTree.documentElement();
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




void ClassroomManager::updateClients( void )
{
	QVector<Client *> vc = visibleClients();

	foreach( Client * cl, vc )
	{
		// update current client
		cl->update();
	}

	QTimer::singleShot( m_clientUpdateInterval, this, SLOT( updateClients() ) );
}




void ClassroomManager::clickedExportToFile( void )
{
	QString outfn = QFileDialog::getSaveFileName( this,
			tr( "Select output-file" ),
			QDir::homePath(),
			tr( "Text files (*.txt)" ) );
	if( outfn == "" )
	{
		return;
	}

	QString output = "# " + QDateTime::currentDateTime().toString() + "\n";

	QVector<Client *> clients = getLoggedInClients();
	foreach( Client * cl, clients )
	{
		output += cl->user() + "\t@ " + cl->name() + " [" + cl->hostname() + "]\n";
	}

	QFile outfile( outfn );
	outfile.open( QFile::WriteOnly );
	outfile.write( output.toUtf8() );
	outfile.close();
}




void ClassroomManager::changeGlobalClientMode( int _mode )
{
	Client::Modes new_mode = static_cast<Client::Modes>( _mode );
	if( new_mode != m_globalClientMode ||
					new_mode == Client::Mode_Overview )
	{
		m_globalClientMode = new_mode;
		QVector<Client *> vc = visibleClients();

		foreach( Client * cl, vc )
		{
			cl->changeMode( m_globalClientMode );
		}
	}
}




void ClassroomManager::powerOnClients( void )
{
	ClientAction action( ClientAction::PowerOn, this );
	action.process( visibleClients(), ClientAction::VisibleClients );
}




/*void ClassroomManager::remoteLogon( void )
{
	ClientAction action( ClientAction::LogonUser, this );
	action.process( visibleClients(), ClientAction::VisibleClients );
}*/




void ClassroomManager::powerDownClients( void )
{
	ClientAction action( ClientAction::PowerDown, this );
	action.process( visibleClients(), ClientAction::VisibleClients );
}




void ClassroomManager::directSupport( void )
{
	const QString h = SupportDialog::getHost( this );
	if( !h.isEmpty() )
	{
		mainWindow()->remoteControlDisplay( h );
	}
}




void ClassroomManager::sendMessage( void )
{
	ClientAction action( ClientAction::SendTextMessage, this );
	action.process( visibleClients(), ClientAction::VisibleClients );
}




const int decor_w = 2;
const int decor_h = 2;


void ClassroomManager::adjustWindows( void )
{
	QVector<Client *> vc = visibleClients();
	if( vc.size() )
	{
		const int avail_w = mainWindow()->workspace()->
						parentWidget()->width();
		const int avail_h = mainWindow()->workspace()->
						parentWidget()->height();
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
		int x_offset = vc[0]->pos().x();
		int y_offset = vc[0]->pos().y();

		foreach( Client * cl, vc )
		{
			if( cl->pos().x() < x_offset )
			{
				x_offset = cl->pos().x();
			}
			if( cl->pos().y() < y_offset )
			{
				y_offset = cl->pos().y();
			}
		}

		float max_rx = 0.0;
		float max_ry = 0.0;
		foreach( Client * cl, vc )
		{
			cl->m_rasterX = roundCorrect( (
					cl->pos().x()-x_offset ) / cw );
			cl->m_rasterY = roundCorrect( (
					cl->pos().y()-y_offset ) / ch );
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

		foreach( Client * cl, vc )
		{
			cl->setFixedSize( nw - decor_w, nh - decor_h );
			cl->move( static_cast<int>( cl->m_rasterX * nw )+1,
				static_cast<int>( cl->m_rasterY * nh )+1 );
		}
		mainWindow()->workspace()->updateGeometry();
	}
}




void ClassroomManager::arrangeWindowsToggle( bool _on )
{
	m_autoArranged = _on;
	if( _on == true )
	{
		arrangeWindows();
	}
}




void ClassroomManager::arrangeWindows( void )
{
	QVector<Client *> vc = visibleClients();
	if( vc.size() )
	{
		const int avail_w = mainWindow()->workspace()->
						parentWidget()->width();
		const int avail_h = mainWindow()->workspace()->
						parentWidget()->height();
		const int w = avail_w;
		const int h = avail_h;
		const float s = sqrt( vc.size() *3* w / (float) (4*h) );
		int win_per_row = (int) ceil( vc.size() / s );
		int win_per_line = (int) ceil( s );
		const int ww = avail_w / win_per_line;
		const int wh = avail_h / win_per_row;

		int i = 0;
		foreach( Client * cl, vc )
		{
			cl->move( ( i % win_per_line ) * ( ww + decor_w ),
						( i / win_per_line ) *
							( wh + decor_h ) );
			cl->setFixedSize( ww, wh );
			++i;
		}
		adjustWindows();
	}
}




void ClassroomManager::resizeClients( const int _new_width )
{
	QVector<Client *> vc = visibleClients();

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

		int x_offset = vc[0]->pos().x();
		int y_offset = vc[0]->pos().y();

		foreach( Client * cl, vc )
		{
			if( cl->pos().x() < x_offset )
			{
				x_offset = cl->pos().x();
			}
			if( cl->pos().y() < y_offset )
			{
				y_offset = cl->pos().y();
			}
		}

		foreach( Client * cl, vc )
		{
			cl->setFixedSize( _new_width, _new_height );
			const int xp = static_cast<int>( (
				cl->pos().x() - x_offset ) /
				cw * ( _new_width + decor_w ) ) + x_offset;
			const int yp = static_cast<int>( (
				cl->pos().y() - y_offset ) /
				ch * ( _new_height + decor_h ) ) + y_offset;
			cl->move( xp, yp );
		}
	}
}




void ClassroomManager::increaseClientSize( void )
{
	QVector<Client *> vc = visibleClients();

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




void ClassroomManager::decreaseClientSize( void )
{
	QVector<Client *> vc = visibleClients();

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




void ClassroomManager::updateIntervalChanged( double seconds )
{
	m_clientUpdateInterval = qRound( seconds * 1000 );
}




void ClassroomManager::itemDoubleClicked( QTreeWidgetItem * _i, int )
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




void ClassroomManager::contextMenuRequest( const QPoint & _pos )
{
	QTreeWidgetItem * i = m_view->itemAt( _pos );
	classRoomItem * cri = dynamic_cast<classRoomItem *>( i );
	classRoom * cr = dynamic_cast<classRoom *>( i );

	QMenu * contextMenu = new QMenu( this );
	QMenu * subMenu;

	if ( m_view->selectedItems().size() > 1 )
	{
		/* multiselection */
		subMenu = new clientMenu( tr( "Actions for selected" ),
			m_clientMenu->actions(), contextMenu );
		connect( subMenu, SIGNAL( triggered( QAction * ) ),
			this, SLOT( clientMenuTriggered( QAction * ) ) );
		contextMenu->addMenu( subMenu );
		contextMenu->addSeparator();
	}
	else if( cri != NULL )
	{
		/* single Client */
		subMenu = new clientMenu( tr( "Actions" ),
			m_clientMenu->actions(), contextMenu, clientMenu::FullMenu );
		connect( subMenu, SIGNAL( triggered( QAction * ) ),
			this, SLOT( clientMenuTriggered( QAction * ) ) );
		contextMenu->addMenu( subMenu );
		contextMenu->addSeparator();
		contextMenu->addActions( m_classRoomItemActionGroup->actions() );
	}
	else if( cr != NULL )
	{
		/* single classroom */
		subMenu = new clientMenu( tr( "Actions for %1" ).arg( cr->text(0) ),
			m_clientMenu->actions(), contextMenu );
		connect( subMenu, SIGNAL( triggered( QAction * ) ),
			cr, SLOT( clientMenuTriggered( QAction * ) ) );
		contextMenu->addMenu( subMenu );
		contextMenu->addSeparator();
		contextMenu->addActions( m_classRoomActionGroup->actions() );
	}
	else
	{ 
		/* no items */
		foreach ( classRoom * cr, m_classRooms )
		{
			subMenu = new clientMenu( tr( "Actions for %1" ).arg( cr->text(0) ),
				m_clientMenu->actions(), contextMenu );
			connect( subMenu, SIGNAL( triggered( QAction * ) ),
				cr, SLOT( clientMenuTriggered( QAction * ) ) );
			contextMenu->addMenu( subMenu );
		}

		contextMenu->addSeparator();
	}

	contextMenu->addActions( m_contextActionGroup->actions() );

	contextMenu->exec( QCursor::pos() );

	delete contextMenu;
}



void ClassroomManager::clientMenuRequest()
{
	bool fullMenu = ( selectedItems().size() == 1 );

	QMenu * menu = new clientMenu( tr( "Actions" ), m_clientMenu->actions(),
					this, fullMenu );
	connect( menu, SIGNAL( triggered( QAction * ) ),
		this, SLOT( clientMenuTriggered( QAction * ) ) );

	menu->exec( QCursor::pos() );

	delete menu;
}




void ClassroomManager::clientMenuTriggered( QAction * _action )
{
	QVector<Client *> clients;

	foreach ( classRoomItem * cri, selectedItems() )
	{
		clients.append( cri->getClient() );
	}

	ClientAction::process( _action, clients );
}




QVector<classRoomItem *> ClassroomManager::selectedItems( void )
{
	QVector<classRoomItem *> vc;

	for( int i = 0; i < m_view->topLevelItemCount(); ++i )
	{
		getSelectedItems( m_view->topLevelItem( i ), vc );
	}

	/* Move the currentItem to the beginning of the list */
	classRoomItem * current = dynamic_cast<classRoomItem *>( m_view->currentItem() );
	if ( vc.contains( current ) )
	{
		vc.remove( vc.indexOf( current ));
		vc.push_front( current );
	}

	return( vc );
}




void ClassroomManager::getSelectedItems( QTreeWidgetItem * _p,
					QVector<classRoomItem *> & _vc,
					bool _add_all )
{
	bool select = _add_all || _p->isSelected();
	if( _p->childCount() )
	{
		for( int i = 0; i < _p->childCount(); ++i )
		{
			getSelectedItems( _p->child( i ), _vc, select );
		}
	}
	else if( dynamic_cast<classRoomItem *>( _p ) && select )
	{
		_vc.push_back( dynamic_cast<classRoomItem *>( _p ) );
	}
}




// slots for client-actions in context-menu
void ClassroomManager::showHideClient( void )
{
	QVector<classRoomItem *> si = selectedItems();

	if( si.size() > 0 )
	{
		bool all_shown = TRUE;
		foreach( classRoomItem * cri, si )
		{
			if( cri->getClient()->isVisible() )
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




void ClassroomManager::editClientSettings( void )
{
	QVector<classRoomItem *> si = selectedItems();

	if( si.size() > 0 )
	{
		foreach( classRoomItem * cri, si )
		{
			ClientSettingsDialog settingsDlg( cri->getClient(),
					mainWindow(),
						cri->parent()->text( 0 ) );
			settingsDlg.exec();
		}
		saveGlobalClientConfig();
		savePersonalConfig();
	}
}




void ClassroomManager::removeClient( void )
{
	QVector<classRoomItem *> si = selectedItems();

	if( si.size() > 0 )
	{
		foreach( classRoomItem * cri, si )
		{
			cri->getClient()->hide();
			m_view->setItemHidden( cri, TRUE );
			m_clientsToRemove.push_back( cri->getClient() );
		}
	}
}




void ClassroomManager::setStateOfClassRoom( classRoom * _cr, bool _shown )
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




QAction * ClassroomManager::addClassRoomToQuickSwitchMenu( classRoom * _cr )
{
	QAction * a = new QAction( _cr->text( 0 ), m_quickSwitchMenu );
	connect( a, SIGNAL( triggered( bool ) ), _cr,
						SLOT( switchToClassRoom() ) );
	m_quickSwitchMenu->insertAction( m_qsmClassRoomSeparator, a );
	return( a );
}




void ClassroomManager::showSelectedClassRooms( void )
{
	foreach( classRoom * cr, m_classRooms )
	{
		if( m_view->isItemSelected( cr ) && cr->childCount() )
		{
			setStateOfClassRoom( cr, TRUE );
		}
	}
}




void ClassroomManager::hideSelectedClassRooms( void )
{
	foreach( classRoom * cr, m_classRooms )
	{
		if( m_view->isItemSelected( cr ) && ( cr )->childCount() )
		{
			setStateOfClassRoom( cr, FALSE );
		}
	}
}




void ClassroomManager::hideAllClassRooms( void )
{
	foreach( classRoom * cr, m_classRooms )
	{
		setStateOfClassRoom( cr, FALSE );
	}
}




void ClassroomManager::editClassRoomName( void )
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




void ClassroomManager::removeClassRoom( void )
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

		removeClassRoom( cr );
	}
}




void ClassroomManager::removeClassRoom( classRoom * cr )
{
		m_view->setItemHidden( cr, TRUE );

		for ( int i = 0 ; i < cr->childCount(); ++i )
		{
			if ( classRoomItem * cri =
					dynamic_cast<classRoomItem *>( cr->child( i ) ) )
			{
				Client * cl = cri->getClient();
				cl->hide();
				m_clientsToRemove.push_back( cl );
			}
			else if ( classRoom * childCr =
					dynamic_cast<classRoom *>( cr->child( i ) ) )
			{
				removeClassRoom( childCr );
			}
		}

		m_classRoomsToRemove.push_back( cr );
}




// slots for general actions in context-menu
void ClassroomManager::addClient( void )
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

	ClientSettingsDialog settingsDlg( NULL, mainWindow(), classroom_name );
	settingsDlg.setWindowTitle( tr( "Add computer" ) );
	settingsDlg.exec();
	saveGlobalClientConfig();
	savePersonalConfig();
}




void ClassroomManager::addClassRoom( void )
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
		foreach( QTreeWidgetItem * item, m_view->selectedItems() )
		{
			sel_cr = dynamic_cast<classRoom *>( item );

			if ( !sel_cr && dynamic_cast<classRoomItem *>( item ) )
			{
				sel_cr = dynamic_cast<classRoom *>( 
					item->parent() );
			}

			if ( sel_cr )
			{
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




void ClassroomManager::hideTeacherClients( void )
{
	QVector<Client *> vc = visibleClients();

	foreach( Client * cl, vc )
	{
		if( cl->type() == Client::Type_Teacher )
		{
			cl->hide();
		}
	}
}




void ClassroomManager::showUserColumn( int _show )
{
	m_view->showColumn( _show ? 2 : 1 );
	m_view->hideColumn( _show ? 1 : 2 );
}




void ClassroomManager::clientVisibleChanged( void )
{
	if( m_autoArranged == true )
	{
		arrangeWindows();
	}
}








classTreeWidget::classTreeWidget( QWidget * _parent ) :
	QTreeWidget( _parent ),
	m_clientPressed( NULL )
{
	setDragEnabled( true );
	setAcceptDrops( true );
	setDropIndicatorShown( true ); 
	setDragDropMode( QAbstractItemView::InternalMove );

	connect( this, SIGNAL( itemSelectionChanged( void ) ),
		this, SLOT( itemSelectionChanged( void ) ) );
}




void classTreeWidget::mousePressEvent( QMouseEvent * _me )
{
	classRoomItem * item = dynamic_cast<classRoomItem *>( itemAt( _me->pos() ) );

	if( item && _me->button() == Qt::LeftButton )
	{
		m_clientPressed = item->getClient();
		m_clientPressed->zoom();
	}

	if( item && _me->button() == Qt::RightButton )
	{
		if ( ! item->isSelected() )
		{
			clearSelection();
			item->setSelected( TRUE );
		}
	}

	QTreeWidget::mousePressEvent( _me );
}




void classTreeWidget::mouseMoveEvent( QMouseEvent * _me )
{
	if ( m_clientPressed ) 
	{
		m_clientPressed->zoomBack();
		m_clientPressed = NULL;
	}

	QTreeWidget::mouseMoveEvent( _me );
}




void classTreeWidget::mouseReleaseEvent( QMouseEvent * _me )
{
	if ( m_clientPressed ) 
	{
		m_clientPressed->zoomBack();
		m_clientPressed = NULL;
	}

	QTreeWidget::mouseReleaseEvent( _me );
}




bool classTreeWidget::droppingOnItself( QTreeWidgetItem * _target )
{
    QList<QTreeWidgetItem *> selected = selectedItems();
    while ( _target )
    {
	    if ( selected.contains( _target ) )
		    return true;
	    _target = dynamic_cast<QTreeWidgetItem * >( _target->parent() );
    }
    return false;
}




void classTreeWidget::dragMoveEvent( QDragMoveEvent * _e )
{
	if ( _e->source() == this ) 
	{
		int clients_selected = 0;
		foreach( QTreeWidgetItem * item, selectedItems() )
		{
			if ( dynamic_cast<classRoomItem *>( item ) )
			{
				clients_selected++;
			}
		}

		QTreeWidgetItem * target = itemAt( _e->pos() );
		
		/* Don't drop clients to the root nor
		 * classroom to its own child */
		if ( ( clients_selected && ! target ) ||
			droppingOnItself( target ) )
		{
			_e->ignore();
		}
		else
		{
			_e->setDropAction( Qt::MoveAction );
			_e->accept();
		}
	}
}




void classTreeWidget::dropEvent( QDropEvent * _e )
{
    if ( _e->source() == this &&
    	dragDropMode() == QAbstractItemView::InternalMove )
    {
		QTreeWidgetItem * target = itemAt( _e->pos() );

		/* Use client's parent as target */
		if ( dynamic_cast<classRoomItem *>( target ))
		{
			target = target->parent();
		}

		/* Workaround for Qt bug #155700 (fixed in Qt 4.3.4) */
		bool sortingEnabled = isSortingEnabled();
		setSortingEnabled( false );

		/* Move selected items */
		foreach ( QTreeWidgetItem * item, selectedItems() )
		{
			if ( item != target )
			{
				QTreeWidgetItem * parent = item->parent();
				if ( parent )
				{
					parent->takeChild( parent->indexOfChild( item ) );
				}
				else
				{
					takeTopLevelItem( indexOfTopLevelItem( item ) );
				}

				if ( target ) 
				{
					target->addChild( item );
				}
				else
				{
					addTopLevelItem( item );
				}
			}
		}

		setSortingEnabled( sortingEnabled );

		_e->accept();
    }
}




/* Update client windows quickly after selections have changed */
void classTreeWidget::itemSelectionChanged( void )
{

	/* update old selections */
	foreach( QTreeWidgetItem * item, m_selectedItems )
	{
		classRoomItem * cri = dynamic_cast<classRoomItem *>( item );
		if ( cri )
		{
			cri->getClient()->update();
		}
	}

	m_selectedItems = selectedItems();

	/* update new selections - some clients may belong to both
	 * lists and are updated here twice. */
	foreach( QTreeWidgetItem * item, m_selectedItems )
	{
		classRoomItem * cri = dynamic_cast<classRoomItem *>( item );
		if ( cri )
		{
			cri->getClient()->update();
		}
	}
}




/* Set to current but don't select it automatically */
void classTreeWidget::setCurrentItem( QTreeWidgetItem * _item )
{
	QModelIndex index = indexFromItem( _item );

	if ( index.isValid() )
	{
		selectionModel()->setCurrentIndex( index,
				QItemSelectionModel::NoUpdate );
	}
}








classRoom::classRoom( const QString & _name,
					ClassroomManager * _classroom_manager,
						QTreeWidgetItem * _parent ) :
	QTreeWidgetItem( _parent, QStringList( _name ) ),
	m_classroomManager( _classroom_manager ),
	m_qsMenuAction( m_classroomManager->addClassRoomToQuickSwitchMenu(
									this ) )
{
}




classRoom::classRoom( const QString & _name,
					ClassroomManager * _classroom_manager,
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




void classRoom::clientMenuTriggered( QAction * _action )
{
	QVector<Client *> clients;
	ClassroomManager::getVisibleClients( this, clients );

	ClientAction::process( _action, clients );
}




void classRoom::switchToClassRoom( void )
{
	m_classroomManager->hideAllClassRooms();
	m_classroomManager->setStateOfClassRoom( this, TRUE );
}










classRoomItem::classRoomItem( Client * _client, QTreeWidgetItem * _parent ) :
	QTreeWidgetItem( _parent, QStringList( _client->name() ) ),
	m_visible( FALSE ),
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

	setFlags( Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
		Qt::ItemIsEnabled );

	setVisible( m_client->isVisible() );
	setText( 1, m_client->hostname() );
	setUser( m_client->user() );
}




classRoomItem::~classRoomItem()
{
	m_client->m_classRoomItem = NULL;
}




void classRoomItem::setVisible( const bool _obs )
{
	m_visible = _obs;
	if( _obs == FALSE )
	{
		setIcon( 0, *s_clientPixmap );
	}
	else
	{
		setIcon( 0, *s_clientObservedPixmap );
	}
}




void classRoomItem::setUser( const QString & _name )
{
	setText( 2, _name );
}



