/*
 * main_window.h - main-window of iTALC
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include <QtGui/QMainWindow>
#include <QtGui/QWhatsThis>


class QMenu;
class QSplashScreen;
class QSplitter;
class QWorkspace;
class QToolBar;

class clientManager;
class configWidget;
class helpWidget;
class italcSideBar;
class isdConnection;
class overviewWidget;
class snapshotList;
class userList;


extern QString MASTER_HOST;



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

	clientManager * getClientManager( void )
	{
		return( m_clientManager );
	}

	isdConnection * localISD( void )
	{
		return( m_localISD );
	}

	static bool ensureConfigPathExists( void );


private slots:
	// slots for actions in help-menu
	void enterWhatsThisMode( void )
	{
		QWhatsThis::enterWhatsThisMode();
	}

	void aboutITALC( void );


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

	QMenu * m_filePopupMenu;
	QMenu * m_actionsPopupMenu;
	QMenu * m_helpPopupMenu;

	QToolBar * m_toolBar;

	QSplitter * m_splitter;

	QWidget * m_sideBarWidget;
	italcSideBar * m_sideBar;
	int m_openedTabInSideBar;

	isdConnection * m_localISD;

	overviewWidget * m_overviewWidget;
	clientManager * m_clientManager;
	userList * m_userList;
	snapshotList * m_snapshotList;
	configWidget * m_configWidget;
	helpWidget * m_helpWidget;


	friend class updateThread;
	friend class clientManager;

} ;


extern QSplashScreen * splashScreen;


#endif
