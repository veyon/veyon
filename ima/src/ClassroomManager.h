/*
* ClassroomManager.h - include file for classroom-manager
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


#ifndef _CLASSROOM_MANAGER_H
#define _CLASSROOM_MANAGER_H

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QPixmap>
#include <QtGui/QMenu>
#include <QtGui/QTreeWidget>
#include <QtGui/QCheckBox>
#include <QtXml/QtXml>

#include "Client.h"
#include "SideBarWidget.h"



class QButtonGroup;
class QMenu;
class QPushButton;

class classTreeWidget;
class classRoom;
class classRoomItem;
class ClientSettingsDialog;
class ConfigWidget;
class MainWindow;


class ClassroomManager : public SideBarWidget
{
	Q_OBJECT
public:
	ClassroomManager( MainWindow * _main_window, QWidget * _parent );
	virtual ~ClassroomManager();


	void doCleanupWork( void );

	void loadGlobalClientConfig( void );
	void saveGlobalClientConfig( void );
	void loadPersonalConfig( void );
	void savePersonalConfig( void );
	void setDefaultWindowsSizeAndPosition( void );

	QVector<Client *> visibleClients( void ) const;
	static void getVisibleClients( QTreeWidgetItem * _p,
						QVector<Client *> & _vv );

	QVector<Client *> getLoggedInClients( void ) const;

	inline int updateInterval( void ) const
	{
		return( m_clientUpdateInterval );
	}

	inline const QString & winCfg( void ) const
	{
		return( m_winCfg );
	}

	inline const QString & toolBarCfg( void ) const
	{
		return( m_toolBarCfg );
	}

	Client::Modes globalClientMode( void ) const
	{
		return( m_globalClientMode );
	}

	int clientDblClickAction( void ) const
	{
		return( m_clientDblClickAction );
	}

	inline void setUpdateIntervalSpinBox( QDoubleSpinBox * _update_interval_sb )
	{
		m_updateIntervalSpinBox = _update_interval_sb;
		m_updateIntervalSpinBox->setValue( m_clientUpdateInterval / 1000.0f );
	}

	inline QMenu * quickSwitchMenu( void )
	{
		return( m_quickSwitchMenu );
	}

	bool showUsername( void ) const
	{
		return( m_showUsernameCheckBox->isChecked() );
	}

	void setStateOfClassRoom( classRoom * _cr, bool _shown );
	QAction * addClassRoomToQuickSwitchMenu( classRoom * _cr );

	void clientVisibleChanged( void );

	void arrangeWindows( void );
	bool isAutoArranged ( )
	{
		return m_autoArranged;
	}

public slots:
	void updateClients( void );

	// slots for context menu
	void clientMenuRequest( void );
	void clientMenuTriggered( QAction * _action );

	// slots for toolbar-actions
	void changeGlobalClientMode( int );
	void sendMessage( void );
	void powerOnClients( void );
	void powerDownClients( void );
	//void remoteLogon( void );
	void directSupport( void );

	// slots for actions in view-menu
	void adjustWindows( void );
	void arrangeWindowsToggle( bool _on );
	void increaseClientSize( void );
	void decreaseClientSize( void );

	// slots for config-widget in side-bar
	void updateIntervalChanged( double seconds );

	void hideAllClassRooms( void );

	void setClientDblClickAction( int _a )
	{
		m_clientDblClickAction = _a;
	}

	void showUserColumn( int _show );

	// Export user list to file
	void clickedExportToFile( void );

private slots:
	void itemDoubleClicked( QTreeWidgetItem * _i, int );
	void contextMenuRequest( const QPoint & _pos );

	// slots for client-actions in context-menu
	void showHideClient( void );
	void editClientSettings( void );
	void removeClient( void );

	// slots for classroom-actions in context-menu
	void showSelectedClassRooms( void );
	void hideSelectedClassRooms( void );
	void editClassRoomName( void );
	void removeClassRoom( void );

	// slots for general actions in context-menu
	void addClient( void );
	void addClassRoom( void );

	void hideTeacherClients( void );


private:
	void setupMenus( void );

	void saveSettingsOfChildren( QDomDocument & _doc, QDomElement & _root,
						QTreeWidgetItem * _lvi,
						bool _is_global_config );

	void getHeaderInformation( const QDomElement & _header );
	void loadTree( classRoom * _parentItem,
					const QDomElement & _parentElement,
					bool _is_global_config );
	void loadMenuElement( QDomElement _e );

	QVector<classRoomItem *> selectedItems( void );
	void getSelectedItems( QTreeWidgetItem * _p,
						QVector<classRoomItem *> & _vv,
						bool _add_all = FALSE );

	void resizeClients( const int _new_width );

	void removeClassRoom( classRoom * _cr );

	classTreeWidget * m_view;

	QVector<classRoom *> m_classRooms;
	QVector<Client *> m_clientsToRemove;
	QVector<classRoom *> m_classRoomsToRemove;

	const QString m_personalConfiguration;
	const QString m_globalClientConfiguration;

	/* context menu: */
	QActionGroup * m_classRoomItemActionGroup;
	QActionGroup * m_classRoomActionGroup;
	QActionGroup * m_contextActionGroup;

	QMenu * m_clientMenu; /* template */
	QVector<QDomNode> m_customMenuConfiguration;

	QDoubleSpinBox * m_updateIntervalSpinBox;
	QMenu * m_quickSwitchMenu;
	QAction * m_qsmClassRoomSeparator;
	Client::Modes m_globalClientMode;

	int m_clientUpdateInterval;
	QString m_winCfg;
	QString m_toolBarCfg;

	int m_clientDblClickAction;

	friend class ClientSettingsDialog;
	friend class ConfigWidget;

	QPushButton * m_exportToFileBtn;
	QCheckBox * m_showUsernameCheckBox;

	bool m_autoArranged;
} ;






class classTreeWidget : public QTreeWidget
{
	Q_OBJECT
public:
	classTreeWidget( QWidget * _parent );
	virtual ~classTreeWidget() { } ;

	void setCurrentItem( QTreeWidgetItem * _item );

private:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void dragMoveEvent( QDragMoveEvent * _e );
	virtual void dropEvent( QDropEvent * _e );

	bool droppingOnItself( QTreeWidgetItem * _target );

	Client * m_clientPressed;
	QList<QTreeWidgetItem *> m_selectedItems;

private slots:
	void itemSelectionChanged( void );

} ;






class classRoom : public QObject, public QTreeWidgetItem
{
	Q_OBJECT
public:
	classRoom( const QString & _name, ClassroomManager * _classroom_manager,
							QTreeWidget * _parent );
	classRoom( const QString & _name, ClassroomManager * _classroom_manager,
						QTreeWidgetItem * _parent );
	virtual ~classRoom();

	void setMenuItemIcon( const QIcon & _icon )
	{
		m_qsMenuAction->setIcon( _icon );
		QFont f = m_qsMenuAction->font();
		f.setBold( !_icon.isNull() );
		m_qsMenuAction->setFont( f );
	}

public slots:
	void switchToClassRoom( void );
	void clientMenuTriggered( QAction * _action );


private slots:


private:
	ClassroomManager * m_classroomManager;
	QAction * m_qsMenuAction;

} ;




class classRoomItem : public QTreeWidgetItem
{
public:
	classRoomItem( Client * _client, QTreeWidgetItem * _parent );
	virtual ~classRoomItem();

	inline Client * getClient( void )
	{
		return( m_client );
	}

	void setVisible( const bool _obs );
	void setUser( const QString & _name );

	inline bool isVisible( void ) const
	{
		return( m_visible );
	}


private:
	bool m_visible;
	Client * m_client;

	static QPixmap * s_clientPixmap;
	static QPixmap * s_clientObservedPixmap;

} ;


#endif
