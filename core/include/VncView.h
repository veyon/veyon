/*
 * VncView.h - VNC viewer widget
 *
 * Copyright (c) 2006-2018 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef VNC_VIEW_H
#define VNC_VIEW_H

#include <QEvent>
#include <QPointer>
#include <QWidget>

#include "KeyboardShortcutTrapper.h"
#include "VeyonVncConnection.h"

class ProgressWidget;
class RemoteControlWidget;
class KeyboardShortcutTrapper;

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
	typedef Modes Mode;

	typedef enum Shortcut
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
	} Shortcut;

	VncView( const QString &host, int port, QWidget *parent, Mode mode );
	~VncView() override;

	bool isViewOnly() const
	{
		return m_viewOnly;
	}

	VeyonVncConnection* vncConnection()
	{
		return m_vncConn;
	}

	QSize scaledSize() const;
	QSize framebufferSize() const
	{
		return m_framebufferSize;
	}
	QSize sizeHint() const override;


public slots:
	void setViewOnly( bool viewOnly );
	void sendShortcut( Shortcut shortcut );


signals:
	void mouseAtTop();
	void keyEvent( int, bool );
	void startConnection();
	void connectionEstablished();
	void sizeHintChanged();


private slots:
	void handleShortcut( KeyboardShortcutTrapper::Shortcut shortcut );
	void updateCursorPos( int x, int y );
	void updateCursorShape( const QPixmap& cursorShape, int xh, int yh );
	void updateImage( int x, int y, int w, int h );
	void updateFramebufferSize( int w, int h );
	void updateConnectionState();

private:
	bool eventFilter( QObject * _obj, QEvent * _event ) override;
	bool event( QEvent * _ev ) override;
	void focusInEvent( QFocusEvent * ) override;
	void focusOutEvent( QFocusEvent * ) override;
	void paintEvent( QPaintEvent * ) override;
	void resizeEvent( QResizeEvent * ) override;

	void keyEventHandler( QKeyEvent * );
	void mouseEventHandler( QMouseEvent * );
	void wheelEventHandler( QWheelEvent * );
	void unpressModifiers();

	bool isScaledView() const;
	float scaleFactor() const;
	QPoint mapToFramebuffer( QPoint pos );
	QRect mapFromFramebuffer( QRect rect );

	void updateLocalCursor();
	void pressKey( unsigned int key );
	void unpressKey( unsigned int key );

	VeyonVncConnection* m_vncConn;

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

} ;

#endif
