/*
 * AccessControlRule.cpp - implementation of the AccessControlRule class
 *
 * Copyright (c) 2016-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

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
	QJsonObject json = QJsonDocument::fromJson( QByteArray::fromBase64( encodedData.toUtf8() ) ).object();

	m_name = json["n"].toString();
	m_description = json["d"].toString();
	m_action = static_cast<Action>( json["a"].toInt() );
	m_entity = static_cast<Entity>( json["e"].toInt() );
	m_invertConditions = json["i"].toBool();

	for( auto conditionValue : json["c"].toArray() )
	{
		QJsonObject conditionObj = conditionValue.toObject();
		Condition condition = static_cast<Condition>(conditionObj["c"].toInt());
		m_conditions[condition] = conditionObj["a"].toVariant();
	}
}



QString AccessControlRule::encode() const
{
	QJsonObject json;

	json["n"] = m_name;
	json["d"] = m_description;
	json["a"] = m_action;
	json["e"] = m_entity;
	json["i"] = m_invertConditions;

	QJsonArray conditions;

	for( auto condition : m_conditions.keys() )
	{
		QJsonObject conditionObj;
		conditionObj["c"] = condition;
		conditionObj["a"] = QJsonValue::fromVariant( m_conditions[condition] );
		conditions.append( conditionObj );
	}

	json["c"] = conditions;

	return QString::fromUtf8( QJsonDocument( json ).toJson( QJsonDocument::Compact ).toBase64() );
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
