/*
 *  RemoteAccessWidget.h - widget containing a VNC view and controls for it
 *
 *  Copyright (c) 2006-2017 Tobias Junghans <tobydox@users.sf.net>
 *
 *  This file is part of Veyon - http://veyon.io
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
class VeyonCoreConnection;
class RemoteAccessWidget;
class ToolButton;

class RemoteAccessWidgetToolBar : public QWidget
{
	Q_OBJECT
public:
	RemoteAccessWidgetToolBar( RemoteAccessWidget * _parent,
							bool _view_only );
	~RemoteAccessWidgetToolBar() override;


public slots:
	void appear();
	void disappear();
	void updateControls( bool viewOnly );


protected:
	void leaveEvent( QEvent * _e ) override;
	void paintEvent( QPaintEvent * _pe ) override;


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

	ToolButton* m_viewOnlyButton;
	ToolButton* m_sendShortcutButton;
	ToolButton* m_screenshotButton;
	ToolButton* m_fullScreenButton;
	ToolButton* m_quitButton;

} ;





class RemoteAccessWidget : public QWidget
{
	Q_OBJECT
public:
	RemoteAccessWidget( const ComputerControlInterface& computerControlInterface, bool _view_only = false );
	~RemoteAccessWidget() override;


public slots:
	void toggleFullScreen( bool );
	void toggleViewOnly( bool viewOnly );
	void takeScreenshot();


protected:
	void enterEvent( QEvent* event ) override;
	void leaveEvent( QEvent* event ) override;
	void resizeEvent( QResizeEvent* event ) override;


private slots:
	void checkKeyEvent( int, bool );
	void updateSize();


private:
	const ComputerControlInterface& m_computerControlInterface;
	VncView* m_vncView;
	VeyonCoreConnection* m_coreConnection;
	RemoteAccessWidgetToolBar* m_toolBar;

	friend class RemoteAccessWidgetToolBar;

} ;

#endif
