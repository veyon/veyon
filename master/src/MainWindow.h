/*
 * MainWindow.h - main window of Veyon Master Application
 *
 * Copyright (c) 2004-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#pragma once

#include <QMainWindow>

#include "Feature.h"

class QButtonGroup;
class QToolButton;

class ComputerSelectPanel;
class ScreenshotManagementPanel;
class VeyonMaster;

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow( VeyonMaster& masterCore, QWidget* parent = nullptr );
	~MainWindow() override;

	static bool initAuthentication();
	static bool initAccessControl();

	VeyonMaster& masterCore()
	{
		return m_master;
	}

	void reloadSubFeatures();

protected:
	void closeEvent( QCloseEvent* event ) override;
	void keyPressEvent( QKeyEvent *e ) override;


private slots:
	void showAboutDialog();

private:
	static int buttonId( const Feature& feature )
	{
		return static_cast<int>( qHash( feature.uid() ) );
	}

	void addFeaturesToToolBar();
	void addSubFeaturesToToolButton( QToolButton* button, const Feature& parentFeature );

	void updateModeButtonGroup();

	Ui::MainWindow* ui;

	VeyonMaster& m_master;

	QButtonGroup* m_modeGroup;

	ComputerSelectPanel* m_computerSelectPanel;
	ScreenshotManagementPanel* m_screenshotManagementPanel;

} ;
