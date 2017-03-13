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
#include <QJsonArray>

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



AccessControlRule::AccessControlRule(const QJsonValueRef &jsonValue) :
	m_name(),
	m_description(),
	m_action( ActionNone ),
	m_entity( EntityNone ),
	m_invertConditions( false ),
	m_conditions()
{
	if( jsonValue.isObject() )
	{
		QJsonObject json = jsonValue.toObject();

		m_name = json["Name"].toString();
		m_description = json["Description"].toString();
		m_action = static_cast<Action>( json["Action"].toInt() );
		m_entity = static_cast<Entity>( json["Entity"].toInt() );
		m_invertConditions = json["InvertConditions"].toBool();

		for( auto conditionValue : json["Conditions"].toArray() )
		{
			QJsonObject conditionObj = conditionValue.toObject();
			Condition condition = static_cast<Condition>(conditionObj["Condition"].toInt());
			m_conditions[condition] = conditionObj["Argument"].toVariant();
		}
	}
}



QJsonObject AccessControlRule::toJson() const
{
	QJsonObject json;

	json["Name"] = m_name;
	json["Description"] = m_description;
	json["Action"] = m_action;
	json["Entity"] = m_entity;
	json["InvertConditions"] = m_invertConditions;

	QJsonArray conditions;

	for( auto condition : m_conditions.keys() )
	{
		QJsonObject conditionObj;
		conditionObj["Condition"] = condition;
		conditionObj["Argument"] = QJsonValue::fromVariant( m_conditions[condition] );
		conditions.append( conditionObj );
	}

	json["Conditions"] = conditions;

	return json;
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
