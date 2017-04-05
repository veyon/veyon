/*
 * ToolButton.h - declaration of class ToolButton
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef TOOL_BUTTON_H
#define TOOL_BUTTON_H

#include <QToolButton>

#include "ItalcCore.h"

class QToolBar;

class ITALC_CORE_EXPORT ToolButton : public QToolButton
{
	Q_OBJECT
public:
	ToolButton( const QPixmap & _pixmap, const QString & _label,
				const QString & _alt_label = QString(),
				const QString & _desc = QString(), QObject * _receiver = nullptr,
				const char * _slot = nullptr );

	ToolButton( const QIcon& icon,
				const QString& label,
				const QString & altLabel,
				const QString & description );
	~ToolButton() override;

	static void setIconOnlyMode( bool enabled );

	static bool iconOnlyMode()
	{
		return s_iconOnlyMode;
	}

	static void setToolTipsDisabled( bool disabled )
	{
		s_toolTipsDisabled = disabled;
	}

	static bool toolTipsDisabled()
	{
		return s_toolTipsDisabled;
	}

	void addTo( QToolBar * );


protected:
	void enterEvent( QEvent * _e ) override;
	void leaveEvent( QEvent * _e ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void paintEvent( QPaintEvent * _pe ) override;


signals:
	void mouseLeftButton();


private slots:
	bool checkForLeaveEvent();


private:
	void updateSize();

	static bool s_toolTipsDisabled;
	static bool s_iconOnlyMode;

	QPixmap m_pixmap;
	QPixmap m_smallPixmap;
	bool m_mouseOver;

	QString m_label;
	QString m_altLabel;
	QString m_descr;

} ;



class ToolButtonTip : public QWidget
{
public:
	ToolButtonTip( const QPixmap & _pixmap, const QString & _title,
				const QString & _description,
				QWidget * _parent, QWidget * _tool_btn = 0 );

	QSize sizeHint( void ) const override;


protected:
	void paintEvent( QPaintEvent * _pe ) override;
	void resizeEvent( QResizeEvent * _re ) override;


private:
	void updateMask( void );

	QPixmap m_icon;
	QString m_title;
	QString m_description;

	QImage m_bg;

	QWidget * m_toolButton;

} ;

#endif
