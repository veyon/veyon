/*
 * Computer.cpp - represents a computer and provides control methods and data
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

#include "Computer.h"

Computer::Computer( NetworkObject::Uid networkObjectUid,
					const QString& name,
					const QString& hostAddress,
					const QString& macAddress,
					const QString& room ) :
	m_networkObjectUid( networkObjectUid ),
	m_name( name ),
	m_hostAddress( hostAddress ),
	m_macAddress( macAddress ),
	m_room( room )
{
}
