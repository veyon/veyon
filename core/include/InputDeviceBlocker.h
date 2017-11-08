/*
 * InputDeviceBlocker.h - class for blocking all input devices
 *
 * Copyright (c) 2016-2017 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef INPUT_DEVICE_BLOCKER_H
#define INPUT_DEVICE_BLOCKER_H

#include "VeyonCore.h"

#ifdef VEYON_BUILD_WIN32
#include <interception.h>
#endif

#include <QByteArray>
#include <QMutex>

#include "SystemService.h"

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT InputDeviceBlocker
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
	void stopHIDService();
	void restoreHIDService();
	void setEmptyKeyMapTable();
	void restoreKeyMapTable();

	static QMutex s_refCntMutex;
	static int s_refCnt;

	bool m_enabled;
	SystemService m_hidService;
	bool m_hidServiceActivated;
#ifdef VEYON_BUILD_LINUX
	void* m_origKeyTable;
	int m_keyCodeMin;
	int m_keyCodeMax;
	int m_keyCodeCount;
	int m_keySymsPerKeyCode;
#endif
#ifdef VEYON_BUILD_WIN32
	InterceptionContext m_interceptionContext;
#endif

} ;

#endif

