/*
 * VncView.h - VNC-viewer-widget
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _VNC_VIEW_H
#define _VNC_VIEW_H

#include <italcconfig.h>

#include <QtCore/QEvent>
#include <QtCore/QThread>
#include <QtGui/QWidget>

#include "ItalcVncConnection.h"
#include "FastQImage.h"


class ProgressWidget;
class RemoteControlWidget;
class SystemKeyTrapper;


class VncView : public QWidget
{
	Q_OBJECT
public:
	VncView( const QString & _host, QWidget * _parent,
							bool _progress_widget );
	virtual ~VncView();

	inline bool viewOnly( void ) const
	{
		return( m_viewOnly );
	}

	inline bool scaledView( void ) const
	{
		return( m_scaledView );
	}

	ItalcVncConnection * vncConnection( void )
	{
		return &m_vncConn;
	}

	QSize scaledSize( const QSize & _default = QSize() ) const;

	QSize framebufferSize() const;
	QSize sizeHint() const;
	QSize minimumSizeHint() const;


public slots:
	void setViewOnly( bool _vo );
	void setScaledView( bool _sv );


signals:
	void mouseAtTop( void );
	void startConnection( void );
	void connectionEstablished( void );


private slots:
	void updateCursorShape( void );
	void updateImage(int x, int y, int w, int h);


private:
	virtual bool eventFilter( QObject * _obj, QEvent * _event );
	virtual bool event( QEvent * _ev );
	virtual void focusInEvent( QFocusEvent * );
	virtual void focusOutEvent( QFocusEvent * );
	virtual void paintEvent( QPaintEvent * );
	virtual void resizeEvent( QResizeEvent * );

	void keyEventHandler( QKeyEvent * );
	void mouseEventHandler( QMouseEvent * );
	void wheelEventHandler( QWheelEvent * );
	void unpressModifiers( void );

	QPoint mapToFramebuffer( const QPoint & _pos );
	QRect mapFromFramebuffer( const QRect & _rect );


	ItalcVncConnection m_vncConn;
	int m_x, m_y, m_w, m_h;
	bool m_repaint;
	FastQImage m_frame;
	bool m_viewOnly;
	bool m_viewOnlyFocus;
	bool m_scaledView;
	bool m_initDone;

	QPoint m_viewOffset;

	int m_buttonMask;
	QMap<unsigned int, bool> m_mods;

	ProgressWidget * m_establishingConnection;

	SystemKeyTrapper * m_sysKeyTrapper;


	friend class remoteControlWidget;

} ;

#endif

