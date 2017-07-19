/*
 * AccessControlRule.cpp - implementation of the AccessControlRule class
 *
 * Copyright (c) 2016-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QDebug>
#include <QJsonArray>

#include "AccessControlRule.h"

AccessControlRule::AccessControlRule() :
	m_name(),
	m_description(),
	m_action( ActionNone ),
	m_parameters(),
	m_invertConditions( false ),
	m_ignoreConditions( false )
{
}



AccessControlRule::AccessControlRule(const AccessControlRule &other) :
	m_name( other.name() ),
	m_description( other.description() ),
	m_action( other.action() ),
	m_parameters( other.parameters() ),
	m_invertConditions( other.areConditionsInverted() ),
	m_ignoreConditions( other.areConditionsIgnored() )
{
}



AccessControlRule::AccessControlRule(const QJsonValue &jsonValue) :
	m_name(),
	m_description(),
	m_action( ActionNone ),
	m_parameters(),
	m_invertConditions( false ),
	m_ignoreConditions( false )
{
	if( jsonValue.isObject() )
	{
		QJsonObject json = jsonValue.toObject();

		m_name = json["Name"].toString();
		m_description = json["Description"].toString();
		m_action = static_cast<Action>( json["Action"].toInt() );
		m_invertConditions = json["InvertConditions"].toBool();
		m_ignoreConditions = json["IgnoreConditions"].toBool();

		auto parameters = json["Parameters"].toArray();

		for( auto parametersValue : parameters )
		{
			QJsonObject parametersObj = parametersValue.toObject();
			auto condition = static_cast<Condition>( parametersObj["Condition"].toInt() );

			m_parameters[condition].enabled = parametersObj["Enabled"].toBool();
			m_parameters[condition].subject = static_cast<Subject>( parametersObj["Subject"].toInt() );
			m_parameters[condition].argument = parametersObj["Argument"].toString();
		}
	}
}



QJsonObject AccessControlRule::toJson() const
{
	QJsonObject json;

	json["Name"] = m_name;
	json["Description"] = m_description;
	json["Action"] = m_action;
	json["InvertConditions"] = m_invertConditions;
	json["IgnoreConditions"] = m_ignoreConditions;

	QJsonArray parameters;

	for( auto it = m_parameters.constBegin(), end = m_parameters.constEnd(); it != end; ++it )
	{
		if( isConditionEnabled( it.key() ) )
		{
			QJsonObject parametersObject;
			parametersObject[QStringLiteral("Condition")] = it.key();
			parametersObject[QStringLiteral("Enabled")] = true;
			parametersObject[QStringLiteral("Subject")] = subject( it.key() );
			parametersObject[QStringLiteral("Argument")] = argument( it.key() );
			parameters.append( parametersObject );
		}
	}

	json[QStringLiteral("Parameters")] = parameters;

	return json;
}
