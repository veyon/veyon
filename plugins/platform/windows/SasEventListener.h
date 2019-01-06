/*
 * SasEventListener.h - header file for SasEventListener class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef SAS_EVENT_LISTENER_H
#define SAS_EVENT_LISTENER_H

#include "VeyonCore.h"

class SasEventListener : public QThread
{
public:
	typedef void (WINAPI *SendSas)(BOOL asUser);

	enum {
		WaitPeriod = 1000
	};

	SasEventListener();
	~SasEventListener() override;

	void stop();

	void run() override;

private:
	HMODULE m_sasLibrary;
	SendSas m_sendSas;
	HANDLE m_sasEvent;
	HANDLE m_stopEvent;

};

#endif
