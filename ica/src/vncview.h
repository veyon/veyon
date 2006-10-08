/*
 * vncview.h - VNC-viewer-widget
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#ifndef _VNCVIEW_H
#define _VNCVIEW_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtCore/QEvent>
#include <QtGui/QWidget>

#include "types.h"
#include "ivs_connection.h"


class progressWidget;
class remoteControlWidget;


class vncView : public QWidget
{
	Q_OBJECT
public:
	vncView( const QString & _host, bool _view_only = TRUE,
							QWidget * _parent = 0 );
	virtual ~vncView();


signals:
	void pointerEvent( Q_UINT16 _x, Q_UINT16 _y, Q_UINT16 _button_mask );
	void keyEvent( Q_UINT32 _key, bool _down );
	void mouseAtTop( void );


private slots:
	void framebufferUpdate( void );


private:
	virtual void customEvent( QEvent * _user );
	virtual bool event( QEvent * );
	virtual void mouseMoveEvent( QMouseEvent * );
	virtual void mousePressEvent( QMouseEvent * );
	virtual void mouseReleaseEvent( QMouseEvent * );
	virtual void mouseDoubleClickEvent( QMouseEvent * );
	virtual void paintEvent( QPaintEvent * );
	virtual void resizeEvent( QResizeEvent * );
	virtual void wheelEvent( QWheelEvent * );

	void keyEvent( QKeyEvent * );
	void mouseEvent( QMouseEvent * );


	ivsConnection * m_connection;
	bool m_viewOnly;

	QPoint m_viewOffset;

	int m_buttonMask;
	QMap<unsigned int, bool> m_mods;

	progressWidget * m_establishingConnection;

	friend class remoteControlWidget;

} ;


#endif

