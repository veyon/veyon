/*
 * local_system.h - misc. platform-specific stuff
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#ifndef _LOCAL_SYSTEM_H
#define _LOCAL_SYSTEM_H

#include <QtCore/QString>

namespace localSystem
{
	void sleep( const int _ms );

	void execInTerminal( const QString & _cmds );

	void sendWakeOnLANPacket( const QString & _mac );

	void powerDown( void );
	void reboot( void );

	void logoutUser( void );

	QString currentUser( void );

}

#endif
