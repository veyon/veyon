/*
 * main_window.h - main-window of iTALC
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


#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H

#include <QtCore/QThread>
#include <QtGui/QButtonGroup>
#include <QtGui/QMainWindow>
#include <QtGui/QToolButton>
#include <QtGui/QWhatsThis>


class QMenu;
class QSplashScreen;
class QSplitter;
class QWorkspace;
class QToolBar;

class classroomManager;
class configWidget;
class supportWidget;
class italcSideBar;
class isdConnection;
class overviewWidget;
class snapshotList;
class userList;


extern QString __demo_network_interface;
extern QString __demo_master_ip;
extern QString __default_domain;
extern int __demo_quality;


class mainWindow : public QMainWindow
{
	Q_OBJECT
public:
	mainWindow();
	virtual ~mainWindow();

	QWorkspace * workspace( void )
	{
		return( m_workspace );
	}

	classroomManager * getClassroomManager( void )
	{
		return( m_classroomManager );
	}

	isdConnection * localISD( void )
	{
		return( m_localISD );
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

	static bool ensureConfigPathExists( void );

	static inline bool atExit( void )
	{
		return( s_atExit );
	}


private slots:
	void enterWhatsThisMode( void )
	{
		QWhatsThis::enterWhatsThisMode();
	}

	void aboutITALC( void );

	void changeGlobalClientMode( int );


private:

	class updateThread : public QThread
	{
	public:
		updateThread( mainWindow * _main_window );

	private:
		virtual void run( void );

		mainWindow * m_mainWindow;

	} * m_updateThread;


	virtual void closeEvent( QCloseEvent * _ce );


	QWorkspace * m_workspace;

	QButtonGroup * m_modeGroup;

	QToolBar * m_toolBar;

	QSplitter * m_splitter;

	QWidget * m_sideBarWidget;
	italcSideBar * m_sideBar;
	int m_openedTabInSideBar;

	isdConnection * m_localISD;

	overviewWidget * m_overviewWidget;
	classroomManager * m_classroomManager;
	userList * m_userList;
	snapshotList * m_snapshotList;
	configWidget * m_configWidget;
	supportWidget * m_supportWidget;

	static bool s_atExit;

	friend class updateThread;
	friend class classroomManager;

} ;


extern QSplashScreen * splashScreen;


#endif
