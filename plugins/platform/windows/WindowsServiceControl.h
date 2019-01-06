/*
 * WindowsService.h - class for managing a Windows service
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <windows.h>

#include "VeyonCore.h"

class WindowsServiceControl : public QObject
{
	Q_OBJECT
public:
	WindowsServiceControl( const QString& name );
	~WindowsServiceControl();

	bool isRegistered();
	bool isRunning();
	bool start();
	bool stop();
	bool install( const QString& filePath, const QString& displayName );
	bool uninstall();
	bool setStartType( int startType );

private:
	bool checkService() const;

	const QString m_name;
	SC_HANDLE m_serviceManager;
	SC_HANDLE m_serviceHandle;

} ;
