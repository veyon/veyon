/*
 * AccessControlRule.cpp - implementation of the AccessControlRule class
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

#include <QDataStream>

#include "AccessControlRule.h"

AccessControlRule::AccessControlRule() :
	m_name(),
	m_description(),
	m_action( ActionNone ),
	m_entity( EntityNone ),
	m_invertConditions( false ),
	m_conditions()
{

}



AccessControlRule::AccessControlRule(const AccessControlRule &other) :
	m_name( other.name() ),
	m_description( other.description() ),
	m_action( other.action() ),
	m_entity( other.entity() ),
	m_invertConditions( other.areConditionsInverted() ),
	m_conditions( other.conditions() )
{

}



AccessControlRule::AccessControlRule(const QString &encodedData) :
	m_name(),
	m_description(),
	m_action( ActionNone ),
	m_entity( EntityNone ),
	m_invertConditions( false ),
	m_conditions()
{
	QByteArray buffer = QByteArray::fromBase64( encodedData.toUtf8() );
	QDataStream ds( buffer );

	qint8 action;
	qint8 entity;
	qint8 invertConditions;
	qint8 conditionCount;

	ds >> m_name;
	ds >> m_description;
	ds >> action;
	ds >> entity;
	ds >> invertConditions;
	ds >> conditionCount;

	for( int i = 0; i < conditionCount; ++i )
	{
		qint8 condition;
		QVariant conditionArgument;
		ds >> condition;
		ds >> conditionArgument;
		m_conditions[static_cast<Condition>(condition)] = conditionArgument;
	}

	m_action = static_cast<Action>( action );
	m_entity = static_cast<Entity>( entity );
	m_invertConditions = static_cast<bool>( invertConditions );
}



QString AccessControlRule::encode() const
{
	QByteArray buffer;
	QDataStream ds( &buffer, QIODevice::ReadWrite );

	ds << m_name;
	ds << m_description;
	ds << static_cast<qint8>( m_action );
	ds << static_cast<qint8>( m_entity );
	ds << static_cast<qint8>( m_invertConditions );
	ds << static_cast<qint8>( m_conditions.count() );

	for( auto condition : m_conditions.keys() )
	{
		ds << static_cast<qint8>( condition );
		ds << m_conditions[condition];
	}

	return QString::fromUtf8( buffer.toBase64() );
}



AccessControlRule::EntityType AccessControlRule::entityType( Entity entity )
{
	switch( entity )
	{
	case EntityAccessingUser:
	case EntityLocalUser:
		return EntityTypeUser;
	case EntityAccessingComputer:
	case EntityLocalComputer:
		return EntityTypeComputer;
	default: break;
	}

	return EntityTypeNone;
}
