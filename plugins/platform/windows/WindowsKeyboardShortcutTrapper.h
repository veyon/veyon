/*
 * WindowsKeyboardShortcutTrapper.cpp - class for trapping shortcuts on Windows
 *
 * Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef WINDOWS_KEYBOARD_SHORTCUT_TRAPPER_H
#define WINDOWS_KEYBOARD_SHORTCUT_TRAPPER_H

#include "KeyboardShortcutTrapper.h"

#include <QMutex>
#include <QTimer>

class WindowsKeyboardShortcutTrapper : public KeyboardShortcutTrapper
{
	Q_OBJECT
public:
	WindowsKeyboardShortcutTrapper( QObject* parent = nullptr );
	~WindowsKeyboardShortcutTrapper() override;

	void setEnabled( bool on ) override;

private slots:
	void forwardTrappedShortcuts();

private:
	enum {
		PollTimerInterval = 10
	};

	static QMutex s_refCntMutex;
	static int s_refCnt;

	bool m_enabled;
	QTimer m_pollTimer;

} ;

#endif
