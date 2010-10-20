/*
 * SystemTrayIconSlave.cpp - an IcaSlave providing the system tray icon
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 * Copyright (c) 2010 Univention GmbH
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

#include "SystemTrayIconSlave.h"
#include "ItalcSlaveManager.h"


SystemTrayIconSlave::SystemTrayIconSlave() :
	IcaSlave(),
	m_systemTrayIcon()
{
	QIcon icon( ":/resources/icon16.png" );
	icon.addFile( ":/resources/icon22.png" );
	icon.addFile( ":/resources/icon32.png" );

	m_systemTrayIcon.setIcon( icon );
	m_systemTrayIcon.show();
}




SystemTrayIconSlave::~SystemTrayIconSlave()
{
}



bool SystemTrayIconSlave::handleMessage( const Ipc::Msg &m )
{
	if( m.cmd() == ItalcSlaveManager::SystemTrayIcon::SetToolTip )
	{
		m_systemTrayIcon.setToolTip(
					m.arg( ItalcSlaveManager::SystemTrayIcon::ToolTipText ) );
		return true;
	}
	else if( m.cmd() == ItalcSlaveManager::SystemTrayIcon::ShowMessage )
	{
		m_systemTrayIcon.showMessage(
					m.arg( ItalcSlaveManager::SystemTrayIcon::Title ),
					m.arg( ItalcSlaveManager::SystemTrayIcon::Text ) );
		return true;
	}

	return false;
}

