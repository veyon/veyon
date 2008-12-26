/*
 * tool_button.h - declaration of class toolButton 
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "fast_qimage.h"


class QToolBar;


class toolButton : public QToolButton
{
	Q_OBJECT
public:
	toolButton( const QPixmap & _pixmap, const QString & _label,
				const QString & _alt_label,
				const QString & _title, 
				const QString & _desc, QObject * _receiver, 
				const char * _slot, QWidget * _parent );
	toolButton( QAction * _a, const QString & _label,
				const QString & _alt_label,
				const QString & _desc, QObject * _receiver, 
				const char * _slot, QWidget * _parent );
	virtual ~toolButton();


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

	void addTo( QToolBar * );


protected:
	virtual void enterEvent( QEvent * _e );
	virtual void leaveEvent( QEvent * _e );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );


signals:
	void mouseLeftButton( void );


private slots:
	bool checkForLeaveEvent( void );
	void updateColorLevel( void );


private:
	void updateSize( void );


	static bool s_toolTipsDisabled;
	static bool s_iconOnlyMode;

	QPixmap m_pixmap;
	fastQImage m_img;
	unsigned char m_colorizeLevel;
	bool m_fadeBack;

	QString m_label;
	QString m_altLabel;
	QString m_title;
	QString m_descr;

} ;




class toolButtonTip : public QWidget
{
public:
	toolButtonTip( const QPixmap & _pixmap, const QString & _title,
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

