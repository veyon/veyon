/*
 * MainWindow.h - main-window of iTALC
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

#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H

#include <QtCore/QReadWriteLock>
#include <QtCore/QThread>
#include <QtGui/QButtonGroup>
#include <QtGui/QMainWindow>
#include <QtGui/QSystemTrayIcon>
#include <QtGui/QToolButton>

#include "ui_MainWindow.h"

#include "Client.h"
#include "SnapshotList.h"


class QMenu;
class QScrollArea;
class QSplashScreen;
class QSplitter;
class ClassroomManager;
class ConfigWidget;
class OverviewWidget;
class RemoteControlWidget;
class UserList;
class ItalcCoreConnection;
class ItalcVncConnection;

extern QString __default_domain;


class MainWindow;


class MainWindowUpdateThread : public QThread
{
	Q_OBJECT
public:
	MainWindowUpdateThread( MainWindow * _main_window );

private slots:
	void update( void );

private:
	virtual void run( void );

	MainWindow * m_mainWindow;

} ;


class clientWorkspace : public QWidget
{
public:
	clientWorkspace( QScrollArea * _parent );
	virtual ~clientWorkspace()
	{
	}

	virtual QSize sizeHint( void ) const;


private:
	virtual void contextMenuEvent( QContextMenuEvent * _event );

	QMenu * m_contextMenu;

	friend class MainWindow;

} ;



class MainWindow : public QMainWindow, public Ui::MainWindow
{
	Q_OBJECT
public:
	MainWindow( int _screen );
	virtual ~MainWindow();

	static bool initAuthentication();

	QWidget * workspace()
	{
		return m_workspace;
	}

	ClassroomManager *getClassroomManager()
	{
		return m_classroomManager;
	}

	ItalcCoreConnection *localICA()
	{
		return m_localICA;
	}

	void checkModeButton( int _id )
	{
		QToolButton * btn = dynamic_cast<QToolButton *>(
						m_modeGroup->button( _id ) );
		if( btn != NULL )
		{
			btn->setChecked( TRUE );
		}
	}

	inline MainToolBar * toolBar()
	{
		return m_toolBar;
	}

	inline SideBar * sideBar()
	{
		return m_sideBar;
	}

	void remoteControlDisplay( const QString & _hostname,
					bool _view_only = FALSE,
					bool _stop_demo_afterwards = FALSE );

	inline bool remoteControlRunning( void )
	{
		QReadLocker rl( &m_rctrlLock );
		return( m_remoteControlWidget != NULL );
	}

protected:
	void keyPressEvent( QKeyEvent *e );


private slots:
	void handleSystemTrayEvent( QSystemTrayIcon::ActivationReason _r );
	void remoteControlClient( QAction * _c );
	void remoteControlWidgetClosed( QObject * );

	void aboutITALC( void );

	void changeGlobalClientMode( int );

	void mapOverview( void )
	{
		changeGlobalClientMode( Client::Mode_Overview );
	}
	void mapFullscreenDemo( void )
	{
		changeGlobalClientMode( Client::Mode_FullscreenDemo );
	}
	void mapWindowDemo( void )
	{
		changeGlobalClientMode( Client::Mode_WindowDemo );
	}
	void mapScreenLock( void )
	{
		changeGlobalClientMode( Client::Mode_Locked );
	}


private:
	virtual void closeEvent( QCloseEvent * _ce );

	MainWindowUpdateThread * m_updateThread;

	clientWorkspace * m_workspace;

	QButtonGroup * m_modeGroup;

	QSystemTrayIcon m_systemTrayIcon;
	QList<QAction *> m_sysTrayActions;

	QWidget * m_sideBarWidget;
	int m_openedTabInSideBar;

	ItalcCoreConnection * m_localICA;

	QReadWriteLock m_rctrlLock;
	RemoteControlWidget * m_remoteControlWidget;
	bool m_stopDemo;
	int m_remoteControlScreen;

	OverviewWidget * m_overviewWidget;
	ClassroomManager * m_classroomManager;
	UserList * m_userList;
	SnapshotList * m_snapshotList;
	ConfigWidget * m_configWidget;

	friend class MainWindowUpdateThread;
	friend class ClassroomManager;

} ;


extern QSplashScreen * splashScreen;


#endif
