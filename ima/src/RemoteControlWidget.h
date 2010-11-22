/*
 *  RemoteControlWidget.h - widget containing a VNC-view and controls for it
 *
 *  Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 *  This file is part of iTALC - http://italc.sourceforge.net
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#ifndef _REMOTE_CONTROL_WIDGET_H
#define _REMOTE_CONTROL_WIDGET_H

#include <QtCore/QTimeLine>
#include <QtGui/QWidget>

#include "FastQImage.h"

class VncView;
class ItalcCoreConnection;
class RemoteControlWidget;


class RemoteControlWidgetToolBar : public QWidget
{
	Q_OBJECT
public:
	RemoteControlWidgetToolBar( RemoteControlWidget * _parent,
							bool _view_only );
	virtual ~RemoteControlWidgetToolBar();


public slots:
	void appear();
	void disappear();


protected:
	virtual void leaveEvent( QEvent * _e );
	virtual void paintEvent( QPaintEvent * _pe );


private slots:
	void updateConnectionAnimation();
	void updatePosition();
	void startConnection();
	void connectionEstablished();


private:
	RemoteControlWidget * m_parent;
	QTimeLine m_showHideTimeLine;
	QTimeLine m_iconStateTimeLine;

	bool m_connecting;
	QImage m_icon;
	FastQImage m_iconGray;

} ;





class RemoteControlWidget : public QWidget
{
	Q_OBJECT
public:
	RemoteControlWidget( const QString & _host, bool _view_only = false );
	virtual ~RemoteControlWidget();

	const QString &host() const
	{
		return m_host;
	}


public slots:
	void lockStudent( bool );
	void toggleFullScreen( bool );
	void toggleViewOnly( bool );
	void takeSnapshot();


protected:
	void updateWindowTitle();
	virtual void enterEvent( QEvent * );
	virtual void leaveEvent( QEvent * );
	virtual void resizeEvent( QResizeEvent * );


private slots:
	void checkKeyEvent( int, bool );
	void lateInit();


private:
	VncView * m_vncView;
	ItalcCoreConnection *m_coreConnection;
	RemoteControlWidgetToolBar * m_toolBar;

	QString m_host;

	friend class RemoteControlWidgetToolBar;

} ;


#endif

