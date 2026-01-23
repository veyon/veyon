/*
 *  RemoteAccessWidget.h - widget containing a VNC view and controls for it
 *
 *  Copyright (c) 2006-2026 Tobias Junghans <tobydox@veyon.io>
 *
 *  This file is part of Veyon - https://veyon.io
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

#pragma once

#include "ComputerControlInterface.h"

#include <QTimeLine>
#include <QWidget>

class QActionGroup;

class VncView;
class VncViewWidget;
class RemoteAccessWidget;
class ToolButton;

// clazy:excludeall=ctor-missing-parent-argument

class RemoteAccessWidgetToolBar : public QWidget
{
	Q_OBJECT
public:
	RemoteAccessWidgetToolBar( RemoteAccessWidget* parent,
							   bool startViewOnly, bool showViewOnlyToggleButton );
	~RemoteAccessWidgetToolBar() override = default;

	void appear();
	void disappear();
	void updateControls( bool viewOnly );
	void updateScreenSelectActions( int newScreen );


protected:
	void leaveEvent( QEvent* event ) override;
	void paintEvent( QPaintEvent* event ) override;


private:
	void updatePosition();
	void updateConnectionState();
	void updateScreens();

	RemoteAccessWidget *m_parent;
	QTimeLine m_showHideTimeLine;

	ToolButton* m_viewOnlyButton;
	ToolButton* m_selectScreenButton;
	ToolButton* m_sendShortcutButton;
	ToolButton* m_screenshotButton;
	ToolButton* m_fullScreenButton;
	ToolButton* m_exitButton;

	QActionGroup* m_screenSelectActions{nullptr};

	static constexpr int ShowHideAnimationDuration = 300;
	static constexpr int DisappearDelay = 500;

} ;





class RemoteAccessWidget : public QWidget
{
	Q_OBJECT
public:
	explicit RemoteAccessWidget( const ComputerControlInterface::Pointer& computerControlInterface,
								 bool startViewOnly, bool showViewOnlyToggleButton );
	~RemoteAccessWidget() override;

	ComputerControlInterface::Pointer computerControlInterface() const
	{
		return m_computerControlInterface;
	}

	VncView* vncView() const;

	void toggleFullScreen( bool );
	void setViewOnly( bool viewOnly );
	void takeScreenshot();

protected:
	bool eventFilter( QObject* object, QEvent* event ) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	void enterEvent( QEnterEvent* event ) override;
#else
	void enterEvent( QEvent* event ) override;
#endif
	void leaveEvent( QEvent* event ) override;
	void resizeEvent( QResizeEvent* event ) override;

private:
	void updateSize();
	void updateRemoteAccessTitle();

	ComputerControlInterface::Pointer m_computerControlInterface;
	VncViewWidget* m_vncView;
	RemoteAccessWidgetToolBar* m_toolBar;

	static constexpr int AppearDelay = 500;

	int m_currentScreen{-1};

Q_SIGNALS:
	void screenChangedInRemoteAccessWidget( int newScreen );

} ;
