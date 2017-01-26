/*
 * NetworkObject.cpp - data class representing a network object
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QHash>

#include "NetworkObject.h"

NetworkObject::NetworkObject(const NetworkObject &other) :
	m_type( other.type() ),
	m_name( other.name() ),
	m_hostAddress( other.hostAddress() ),
	m_macAddress( other.macAddress() )
{
}



NetworkObject::NetworkObject(NetworkObject::Type type,
							 const QString &name,
							 const QString &hostAddress,
							 const QString &macAddress) :
	m_type( type ),
	m_name( name ),
	m_hostAddress( hostAddress ),
	m_macAddress( macAddress )
{
}



bool NetworkObject::operator ==(const NetworkObject &other) const
{
	return type() == other.type() &&
			name() == other.name() &&
			hostAddress() == other.hostAddress() &&
			macAddress() == other.macAddress();
}



NetworkObject::Uid NetworkObject::uid() const
{
	return qHash( type() ) + qHash( name() ) + qHash( hostAddress() );
}
