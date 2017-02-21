/*
 *  RemoteAccessWidget.h - widget containing a VNC view and controls for it
 *
 *  Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef REMOTE_ACCESS_WIDGET_H
#define REMOTE_ACCESS_WIDGET_H

#include <QTimeLine>
#include <QWidget>

class VncView;
class ComputerControlInterface;
class ItalcCoreConnection;
class RemoteAccessWidget;


class RemoteAccessWidgetToolBar : public QWidget
{
	Q_OBJECT
public:
	RemoteAccessWidgetToolBar( RemoteAccessWidget * _parent,
							bool _view_only );
	virtual ~RemoteAccessWidgetToolBar();


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
	RemoteAccessWidget * m_parent;
	QTimeLine m_showHideTimeLine;
	QTimeLine m_iconStateTimeLine;

	bool m_connecting;
	QPixmap m_icon;

} ;





class RemoteAccessWidget : public QWidget
{
	Q_OBJECT
public:
	RemoteAccessWidget( const ComputerControlInterface& computerControlInterface, bool _view_only = false );
	virtual ~RemoteAccessWidget();


public slots:
	void lockStudent( bool );
	void toggleFullScreen( bool );
	void toggleViewOnly( bool );
	void takeSnapshot();
	void updateWindowTitle();


protected:
	virtual void enterEvent( QEvent* event );
	virtual void leaveEvent( QEvent* event );
	virtual void resizeEvent( QResizeEvent* event );


private slots:
	void checkKeyEvent( int, bool );
	void updateSize();


private:
	const ComputerControlInterface& m_computerControlInterface;
	VncView* m_vncView;
	ItalcCoreConnection* m_coreConnection;
	RemoteAccessWidgetToolBar* m_toolBar;

	friend class RemoteAccessWidgetToolBar;

} ;


#endif

