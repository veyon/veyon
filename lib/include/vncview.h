/*
 * vncview.h - VNC-viewer-widget
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include <QtCore/QThread>
#include <QtGui/QWidget>

#include "types.h"
#include "ivs_connection.h"


class progressWidget;
class remoteControlWidget;
class systemKeyTrapper;
class vncWorker;


class IC_DllExport vncView : public QWidget
{
	Q_OBJECT
public:
	vncView( const QString & _host, QWidget * _parent = 0 );
	virtual ~vncView();

	inline bool viewOnly( void ) const
	{
		return( m_viewOnly );
	}


public slots:
	void setViewOnly( bool _vo );


signals:
	void pointerEvent( Q_UINT16 _x, Q_UINT16 _y, Q_UINT16 _button_mask );
	void keyEvent( Q_UINT32 _key, bool _down );
	void mouseAtTop( void );
	void startConnection( void );
	void connectionEstablished( void );


private slots:
	void framebufferUpdate( void );
	void updateCursorShape( void );

private:
	virtual void customEvent( QEvent * _user );
	virtual bool event( QEvent * );
	virtual void focusInEvent( QFocusEvent * );
	virtual void focusOutEvent( QFocusEvent * );
	virtual void mouseMoveEvent( QMouseEvent * );
	virtual void mousePressEvent( QMouseEvent * );
	virtual void mouseReleaseEvent( QMouseEvent * );
	virtual void mouseDoubleClickEvent( QMouseEvent * );
	virtual void paintEvent( QPaintEvent * );
	virtual void resizeEvent( QResizeEvent * );
	virtual void wheelEvent( QWheelEvent * );

	void keyEvent( QKeyEvent * );
	void mouseEvent( QMouseEvent * );
	void unpressModifiers( void );


	ivsConnection * m_connection;
	bool m_viewOnly;
	bool m_viewOnlyFocus;

	QPoint m_viewOffset;

	int m_buttonMask;
	QMap<unsigned int, bool> m_mods;

	progressWidget * m_establishingConnection;

	systemKeyTrapper * m_sysKeyTrapper;


	friend class remoteControlWidget;
	friend class vncWorker;

} ;




class vncWorker : public QObject
{
	Q_OBJECT
public:
	vncWorker( vncView * _vv );
	~vncWorker();


private slots:
	void framebufferUpdate( void );
	void sendPointerEvent( Q_UINT16 _x, Q_UINT16 _y,
							Q_UINT16 _button_mask );
	void sendKeyEvent( Q_UINT32 _key, bool _down );


private:
	vncView * m_vncView;

} ;




class vncViewThread : public QThread
{
	Q_OBJECT
public:
	vncViewThread( vncView * _vv );
	virtual ~vncViewThread()
	{
	}


private:
	virtual void run( void );

	vncView * m_vncView;

} ;


#endif

