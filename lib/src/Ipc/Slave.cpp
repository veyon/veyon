/*
 * IpcSlave.cpp - class Ipc::Slave providing communication with Ipc::Master
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

#include <QtCore/QCoreApplication>

#include "Ipc/Slave.h"

namespace Ipc
{

Slave::Slave( const Ipc::Id &masterId, const Ipc::Id &slaveId) :
	QLocalSocket()
{
	connectToServer( masterId );
	if( waitForConnected( 5000 ) &&
		Ipc::Msg().receive( this ).cmd() == Ipc::Commands::Identify )
	{
		connect( this, SIGNAL( readyRead() ),
					this, SLOT( receiveMessage() ) );
		Ipc::Msg( Ipc::Commands::Identify ).
				addArg( "id", slaveId ).
			send( this );
	}
}




void Slave::receiveMessage()
{
	while( bytesAvailable() > 0 )
	{
		Ipc::Msg m;
		if( m.receive( this ).isValid() )
		{
			if( handleMessage( m ) )
			{
				// ok
			}
			else if( m.cmd() == Ipc::Commands::Quit )
			{
				QCoreApplication::quit();
			}
			else
			{
				Ipc::Msg( Ipc::Commands::UnknownCommand ).
								addArg( "Command", m.cmd() ).send( this );
			}
		}
	}
}

}
