/*
 * MainWindow.h - main window of iTALC Master Application
 *
 * Copyright (c) 2004-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QtCore/QThread>
#include <QPointer>
#include <QButtonGroup>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QToolButton>

#include "ComputerManager.h"

class QMenu;
class SideBar;
class MainToolBar;
class MasterCore;
class ComputerManagementView;
class ConfigWidget;
class WelcomeWidget;
class RemoteControlWidget;
class ItalcCoreConnection;
class SnapshotManagementWidget;
class Feature;

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow( MasterCore& masterCore );
	virtual ~MainWindow();

	static bool initAuthentication();
/*
	ItalcCoreConnection *localICA()
	{
		return m_localICA;
	}*/

	void checkModeButton( int _id )
	{
		QToolButton * btn = dynamic_cast<QToolButton *>(
						m_modeGroup->button( _id ) );
		if( btn != NULL )
		{
			btn->setChecked( true );
		}
	}

	MainToolBar* toolBar();

	SideBar* sideBar();

	void remoteControlDisplay( const QString& hostname,
								bool viewOnly = false,
								bool stopDemoAfterwards = false );


protected:
	void keyPressEvent( QKeyEvent *e ) override;


private slots:
	void handleSystemTrayEvent( QSystemTrayIcon::ActivationReason _r );


private:
	void addFeaturesToToolBar();
	void addFeaturesToSystemTrayMenu();

	void updateModeButtonGroup();

	Ui::MainWindow* ui;

	MasterCore& m_masterCore;

	QButtonGroup* m_modeGroup;

	QSystemTrayIcon m_systemTrayIcon;
	QList<QAction *> m_sysTrayActions;

	WelcomeWidget* m_welcomeWidget;
	ComputerManagementView* m_computerManagementView;
	SnapshotManagementWidget* m_snapshotManagementWidget;
	ConfigWidget* m_configWidget;

} ;

#endif
