/*
 * VncView.h - VNC viewer widget
 *
 * Copyright (c) 2006-2020 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#pragma once

#include <QEvent>
#include <QPointer>
#include <QTimer>
#include <QWidget>

#include "KeyboardShortcutTrapper.h"
#include "VncConnection.h"

class ProgressWidget;
class RemoteControlWidget;
class KeyboardShortcutTrapper;
class VeyonConnection;

class VEYON_CORE_EXPORT VncView : public QWidget
{
	Q_OBJECT
public:
	enum Modes
	{
		RemoteControlMode,
		DemoMode,
		NumModes
	} ;
	using Mode = Modes;

	enum Shortcut
	{
		ShortcutCtrlAltDel,
		ShortcutCtrlEscape,
		ShortcutAltTab,
		ShortcutAltF4,
		ShortcutWinTab,
		ShortcutWin,
		ShortcutMenu,
		ShortcutAltCtrlF1,
		ShortcutCount
	} ;

	VncView( const QString &host, int port, QWidget *parent, Mode mode );
	~VncView() override;

	bool isViewOnly() const
	{
		return m_viewOnly;
	}

	VncConnection* vncConnection()
	{
		return m_vncConn;
	}

	QSize scaledSize() const;
	QSize framebufferSize() const
	{
		return m_framebufferSize;
	}
	QSize sizeHint() const override;


public Q_SLOTS:
	void setViewOnly( bool viewOnly );
	void sendShortcut( Shortcut shortcut );


Q_SIGNALS:
	void mouseAtBorder();
	void keyEvent( unsigned int, bool );
	void startConnection();
	void connectionEstablished();
	void sizeHintChanged();


private:
	void handleShortcut( KeyboardShortcutTrapper::Shortcut shortcut );
	void updateCursorPos( int x, int y );
	void updateCursorShape( const QPixmap& cursorShape, int xh, int yh );
	void updateImage( int x, int y, int w, int h );
	void updateFramebufferSize( int w, int h );
	void updateConnectionState();

	bool eventFilter( QObject* obj, QEvent* event ) override;
	bool event( QEvent* event ) override;
	void focusInEvent( QFocusEvent* event ) override;
	void focusOutEvent( QFocusEvent* event ) override;
	void paintEvent( QPaintEvent* event ) override;
	void resizeEvent( QResizeEvent* event ) override;

	void keyEventHandler( QKeyEvent* event );
	void mouseEventHandler( QMouseEvent* event );
	void wheelEventHandler( QWheelEvent* event );
	void unpressModifiers();

	bool isScaledView() const;
	qreal scaleFactor() const;
	QPoint mapToFramebuffer( QPoint pos );
	QRect mapFromFramebuffer( QRect rect );

	void updateLocalCursor();
	void pressKey( unsigned int key );
	void unpressKey( unsigned int key );

	VncConnection* m_vncConn;
	VeyonConnection* m_veyonConnection;

	Mode m_mode;
	QPixmap m_cursorShape;
	int m_cursorX;
	int m_cursorY;
	QSize m_framebufferSize;
	int m_cursorHotX;
	int m_cursorHotY;
	bool m_viewOnly;
	bool m_viewOnlyFocus;
	bool m_initDone;

	int m_buttonMask;
	QMap<unsigned int, bool> m_mods;

	ProgressWidget* m_establishingConnectionWidget;

	KeyboardShortcutTrapper* m_keyboardShortcutTrapper;

	static constexpr int MouseBorderSignalDelay = 500;
	QTimer m_mouseBorderSignalTimer;

} ;
