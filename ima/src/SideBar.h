/*
 * SideBar.h - side bar in MainWindow
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef _SIDE_BAR_H
#define _SIDE_BAR_H

#include <QtCore/QMap>
#include <QtGui/QButtonGroup>
#include <QtGui/QToolBar>

class QToolButton;
class SideBarWidget;


class SideBar : public QToolBar
{
	Q_OBJECT
public:
	SideBar( QWidget * _parent );
	virtual ~SideBar();

	void appendTab( SideBarWidget * _sbw );
	int activeTab() const;

	QList<QAbstractButton *> tabs() const
	{
		return m_btnGroup.buttons();
	}


private slots:
	void toggleButton( QAction * _a );
	void toggleButton( QAbstractButton * _btn );


private:
	virtual void contextMenuEvent( QContextMenuEvent * _e );
	virtual void paintEvent( QPaintEvent * _pe );

	QButtonGroup m_btnGroup;
	typedef QMap<QToolButton *, QWidget *> ButtonMap;
	ButtonMap m_widgets;

} ;

#endif
