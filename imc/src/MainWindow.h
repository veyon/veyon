/*
 * MainWindow.h - main window of the iTALC Management Console
 *
 * Copyright (c) 2010-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtGui/QMainWindow>

class QAbstractButton;

namespace Ui { class MainWindow; } ;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow();
	virtual ~MainWindow();

	void reset( bool onlyUI = false );
	void apply();


private slots:
	void configurationChanged();
	void resetOrApply( QAbstractButton *btn );
	void startService();
	void stopService();
	void updateServiceControl();
	void openLogFileDirectory();
	void clearLogFiles();
	void openGlobalConfig();
	void openPersonalConfig();
	void openSnapshotDirectory();
	void openPublicKeyBaseDir();
	void openPrivateKeyBaseDir();
	void loadSettingsFromFile();
	void saveSettingsToFile();
	void launchKeyFileAssistant();
	void manageACLs();
	void testLogonAuthentication();
	void generateBugReportArchive();
	void aboutItalc();


private:
	virtual void closeEvent( QCloseEvent *closeEvent );
	void serviceControlWithProgressBar( const QString &title,
										const QString &arg );

	bool isServiceRunning();

	Ui::MainWindow *ui;
	bool m_configChanged;

} ;

#endif
