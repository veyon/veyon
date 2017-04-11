/*
 * MainWindow.h - main window of Veyon Master Application
 *
 * Copyright (c) 2004-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <QMainWindow>
#include <QSystemTrayIcon>

class QButtonGroup;
class MasterCore;
class ComputerManagementView;
class SnapshotManagementWidget;

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow( MasterCore& masterCore );
	~MainWindow() override;

	static bool initAuthentication();

	MasterCore& masterCore()
	{
		return m_masterCore;
	}


protected:
	void closeEvent( QCloseEvent* event ) override;
	void keyPressEvent( QKeyEvent *e ) override;


private slots:
	void handleSystemTrayEvent( QSystemTrayIcon::ActivationReason _r );
	void showAboutDialog();

private:
	void addFeaturesToToolBar();
	void addFeaturesToSystemTrayMenu();

	void updateModeButtonGroup();

	Ui::MainWindow* ui;

	MasterCore& m_masterCore;

	QButtonGroup* m_modeGroup;

	QSystemTrayIcon m_systemTrayIcon;
	QList<QAction *> m_sysTrayActions;

	ComputerManagementView* m_computerManagementView;
	SnapshotManagementWidget* m_snapshotManagementWidget;

} ;

#endif
