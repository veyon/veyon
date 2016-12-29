/*
 * InputDeviceBlocker.h - class for blocking all input devices
 *
 * Copyright (c) 2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef INPUT_DEVICE_BLOCKER_H
#define INPUT_DEVICE_BLOCKER_H

#include <italcconfig.h>

#ifdef ITALC_BUILD_WIN32
#include <interception.h>
#endif

#include <QtCore/QByteArray>
#include <QtCore/QMutex>


class InputDeviceBlocker
{
public:
	InputDeviceBlocker( bool enable = true );
	~InputDeviceBlocker();

	void setEnabled( bool on );
	bool isEnabled() const
	{
		return m_enabled;
	}


private:
	void enableInterception();
	void disableInterception();
	void saveKeyMapTable();
	void setEmptyKeyMapTable();
	void restoreKeyMapTable();

	static QMutex s_refCntMutex;
	static int s_refCnt;

	bool m_enabled;
#ifdef ITALC_BUILD_LINUX
	QByteArray m_origKeyTable;
#endif
#ifdef ITALC_BUILD_WIN32
	InterceptionContext m_interceptionContext;
#endif

} ;

#endif

