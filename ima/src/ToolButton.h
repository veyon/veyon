/*
 * ToolButton.h - declaration of class ToolButton
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _TOOL_BUTTON_H
#define _TOOL_BUTTON_H 

#include <QtGui/QToolButton>

#include "FastQImage.h"


class QToolBar;


class ToolButton : public QToolButton
{
	Q_OBJECT
public:
	ToolButton( const QPixmap & _pixmap, const QString & _label,
				const QString & _alt_label,
				const QString & _title, 
				const QString & _desc, QObject * _receiver, 
				const char * _slot, QWidget * _parent );
	ToolButton( QAction * _a, const QString & _label,
				const QString & _alt_label,
				const QString & _desc, QObject * _receiver, 
				const char * _slot, QWidget * _parent );
	virtual ~ToolButton();


#ifndef ITALC3
	static void setIconOnlyMode( bool _enabled );

	static bool iconOnlyMode( void )
	{
		return( s_iconOnlyMode );
	}

	static void setToolTipsDisabled( bool _disabled )
	{
		s_toolTipsDisabled = _disabled;
	}

	static bool toolTipsDisabled( void )
	{
		return( s_toolTipsDisabled );
	}
#endif

	void addTo( QToolBar * );


protected:
	virtual void enterEvent( QEvent * _e );
	virtual void leaveEvent( QEvent * _e );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );


signals:
	void mouseLeftButton();


private slots:
	bool checkForLeaveEvent();


private:
	void updateSize();


#ifndef ITALC3
	static bool s_toolTipsDisabled;
	static bool s_iconOnlyMode;
#endif

	QPixmap m_pixmap;
	FastQImage m_img;
	bool m_mouseOver;

	QString m_label;
	QString m_altLabel;
	QString m_title;
	QString m_descr;

} ;




class ToolButtonTip : public QWidget
{
public:
	ToolButtonTip( const QPixmap & _pixmap, const QString & _title,
				const QString & _description,
				QWidget * _parent, QWidget * _tool_btn = 0 );

	virtual QSize sizeHint( void ) const;


protected:
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re );


private:
	void updateMask( void );

	QImage m_icon;
	QString m_title;
	QString m_description;

	QImage m_bg;

	QWidget * m_toolButton;

} ;



#endif

