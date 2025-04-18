/*
 * MainWindow.h - main window of the Veyon Configurator
 *
 * Copyright (c) 2010-2025 Tobias Junghans <tobydox@veyon.io>
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

class QAbstractButton;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow( QWidget* parent = nullptr );
	~MainWindow() override;

	void reset( bool onlyUI = false );
	void apply();

protected:
	void closeEvent( QCloseEvent* event ) override;
	void resizeEvent( QResizeEvent* event ) override;

private Q_SLOTS:
	void configurationChanged();
	void resetOrApply( QAbstractButton *btn );
	void loadSettingsFromFile();
	void saveSettingsToFile();
	void resetConfiguration();
	void aboutVeyon();

private:
	static QString windowGeometryKey()
	{
		return QStringLiteral("Configurator/WindowGeometry");
	}

	void updateSizes();
	void updateView();

	void switchToStandardView();
	void switchToAdvancedView();

	void loadConfigurationPagePlugins();

	Ui::MainWindow *ui;
	bool m_configChanged;

} ;
