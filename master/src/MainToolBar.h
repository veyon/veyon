/*
 * MainToolBar.h - MainToolBar for MainWindow
 *
 * Copyright (c) 2007-2017 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifndef MAIN_TOOL_BAR_H
#define MAIN_TOOL_BAR_H

#include <QToolBar>

class MainWindow;

class MainToolBar : public QToolBar
{
	Q_OBJECT
public:
	MainToolBar( QWidget* parent );
	~MainToolBar() override;


private slots:
	void toggleToolTips();
	void toggleIconMode();


private:
	void contextMenuEvent( QContextMenuEvent* event ) override;
	void paintEvent( QPaintEvent* event ) override;

	MainWindow* m_mainWindow;

} ;

#endif
