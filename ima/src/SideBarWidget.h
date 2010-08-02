/*
 * SideBarWidget.h - base-class for all side-bar-widgets
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

#ifndef _SIDE_BAR_WIDGET_H
#define _SIDE_BAR_WIDGET_H

#include <QtGui/QWidget>
#include <QtGui/QPixmap>


class MainWindow;


class SideBarWidget : public QWidget
{
public:
	SideBarWidget( const QPixmap & _icon, const QString & _title,
			const QString & _description,
				MainWindow * _main_window, QWidget * _parent );
	virtual ~SideBarWidget();

	inline const QPixmap & icon( void ) const
	{
		return m_icon;
	}

	inline const QString & title( void ) const
	{
		return m_title;
	}

	inline const QString & description( void ) const
	{
		return m_description;
	}


protected:
	virtual void paintEvent( QPaintEvent * _pe );

	inline QWidget * contentParent()
	{
		return m_contents;
	}

	inline MainWindow * mainWindow()
	{
		return m_mainWindow;
	}


private:
	MainWindow * m_mainWindow;
	QWidget * m_contents;
	QPixmap m_icon;
	QString m_title;
	QString m_description;

} ;


#endif
