/*
 * DesktopServiceObject.cpp - data class representing a desktop service object
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QJsonObject>

#include "DesktopServiceObject.h"


DesktopServiceObject::DesktopServiceObject( const DesktopServiceObject& other ) :
	m_type( other.type() ),
	m_name( other.name() ),
	m_path( other.path() ),
	m_uid( other.uid() )
{
}



DesktopServiceObject::DesktopServiceObject( DesktopServiceObject::Type type,
											const Name& name,
											const QString& path,
											Uid uid ) :
	m_type( type ),
	m_name( name ),
	m_path( path ),
	m_uid( uid )
{
	if( m_uid.isNull() )
	{
		m_uid = QUuid::createUuid();
	}
}



DesktopServiceObject::DesktopServiceObject( const QJsonObject& jsonObject ) :
	m_type( static_cast<Type>( jsonObject.value( QStringLiteral( "Type" ) ).toInt() ) ),
	m_name( jsonObject.value( QStringLiteral( "Name" ) ).toString() ),
	m_path( jsonObject.value( QStringLiteral( "Path" ) ).toString() ),
	m_uid( jsonObject.value( QStringLiteral( "Uid" ) ).toString() )
{
}



bool DesktopServiceObject::operator==( const DesktopServiceObject& other ) const
{
	return uid() == other.uid();
}



QJsonObject DesktopServiceObject::toJson() const
{
	QJsonObject json;
	json[QStringLiteral("Type")] = static_cast<int>( type() );
	json[QStringLiteral("Name")] = name();
	json[QStringLiteral("Path")] = path();
	json[QStringLiteral("Uid")] = uid().toString();

	return json;
}
