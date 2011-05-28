/*
 * VncView.h - VNC viewer widget
 *
 * Copyright (c) 2006-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
	enum Modes
	{
		RemoteControlMode,
		DemoMode,
		NumModes
	} ;
	typedef Modes Mode;

	VncView( const QString &host, QWidget *parent, Mode mode );
	virtual ~VncView();

	inline bool isViewOnly() const
	{
		return m_viewOnly;
	}

	inline bool isScaledView() const
	{
		return m_scaledView;
	}

	ItalcVncConnection * vncConnection()
	{
		return &m_vncConn;
	}

	QSize scaledSize() const;
	QSize framebufferSize() const
	{
		return m_framebufferSize;
	}
	QSize sizeHint() const;


public slots:
	void setViewOnly( bool _vo );
	void setScaledView( bool _sv );


signals:
	void mouseAtTop();
	void keyEvent( int, bool );
	void startConnection();
	void connectionEstablished();
	void sizeHintChanged();


private slots:
	void checkKeyEvent( unsigned int key, bool pressed );
	void updateCursorPos( int x, int y );
	void updateCursorShape( const QImage &cursorShape, int xh, int yh );
	void updateImage( int x, int y, int w, int h );
	void updateSizeHint( int w, int h );


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
	void unpressModifiers();

	QPoint mapToFramebuffer( const QPoint & _pos );
	QRect mapFromFramebuffer( const QRect & _rect );

	void updateLocalCursor();


	ItalcVncConnection m_vncConn;

	Mode m_mode;
	int m_x, m_y, m_w, m_h;
	bool m_repaint;
	FastQImage m_frame;
	QImage m_cursorShape;
	int m_cursorX;
	int m_cursorY;
	QSize m_framebufferSize;
	int m_cursorHotX;
	int m_cursorHotY;
	bool m_viewOnly;
	bool m_viewOnlyFocus;
	bool m_scaledView;
	bool m_initDone;

	int m_buttonMask;
	QMap<unsigned int, bool> m_mods;

	ProgressWidget * m_establishingConnection;

	SystemKeyTrapper * m_sysKeyTrapper;


	friend class remoteControlWidget;

} ;

#endif

