/*
 * tool_button.h - declaration of class toolButton 
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtGui/QColor>


class toolButtonTip : public QWidget
{
	Q_OBJECT
public:
	toolButtonTip( const QPixmap & _pixmap, const QString & _title,
				const QString & _description,
							QWidget * _parent );

	virtual QSize sizeHint( void ) const;


protected:
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re );


private slots:
	void updateMask( void );


private:
	QImage m_icon;
	QString m_title;
	QString m_description;

	QPixmap m_bg;
	int m_dissolveSize;

} ;



class toolButton : public QToolButton
{
	Q_OBJECT
public:
	toolButton( const QPixmap & _pixmap, const QString & _title,
			const QString & _description,
			QObject * _receiver, const char * _slot,
			QWidget * _parent );

	virtual ~toolButton();


protected:
	virtual void enterEvent( QEvent * _ev );
	virtual void leaveEvent( QEvent * _ev );
	virtual void paintEvent( QPaintEvent * _pe );


private:
	toolButtonTip * m_toolButtonTip;

	QPixmap m_pixmap;
	QString m_title;
	QString m_description;

	bool m_mouseOver;


signals:
	void mouseLeftButton( void );

} ;

#endif

