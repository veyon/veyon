/*
 * AccessControlRule.cpp - implementation of the AccessControlRule class
 *
 * Copyright (c) 2016-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QJsonArray>

#include "AccessControlRule.h"

AccessControlRule::AccessControlRule() :
	m_name(),
	m_description(),
	m_action( Action::None ),
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
	m_action( Action::None ),
	m_parameters(),
	m_invertConditions( false ),
	m_ignoreConditions( false )
{
	if( jsonValue.isObject() )
	{
		QJsonObject json = jsonValue.toObject();

		m_name = json[QStringLiteral("Name")].toString();
		m_description = json[QStringLiteral("Description")].toString();
		m_action = static_cast<Action>( json[QStringLiteral("Action")].toInt() );
		m_invertConditions = json[QStringLiteral("InvertConditions")].toBool();
		m_ignoreConditions = json[QStringLiteral("IgnoreConditions")].toBool();

		auto parameters = json[QStringLiteral("Parameters")].toArray();

		for( auto parametersValue : parameters )
		{
			QJsonObject parametersObj = parametersValue.toObject();
			auto condition = static_cast<Condition>( parametersObj[QStringLiteral("Condition")].toInt() );

			m_parameters[condition].enabled = parametersObj[QStringLiteral("Enabled")].toBool();
			m_parameters[condition].subject = static_cast<Subject>( parametersObj[QStringLiteral("Subject")].toInt() );
			m_parameters[condition].argument = parametersObj[QStringLiteral("Argument")].toString();
		}
	}
}



AccessControlRule& AccessControlRule::operator=( const AccessControlRule& other )
{
	m_name = other.name();
	m_description = other.description();
	m_action = other.action();
	m_parameters = other.parameters();
	m_invertConditions = other.areConditionsInverted();
	m_ignoreConditions = other.areConditionsIgnored();

	return *this;
}



QJsonObject AccessControlRule::toJson() const
{
	QJsonObject json;

	json[QStringLiteral("Name")] = m_name;
	json[QStringLiteral("Description")] = m_description;
	json[QStringLiteral("Action")] = static_cast<int>( m_action );
	json[QStringLiteral("InvertConditions")] = m_invertConditions;
	json[QStringLiteral("IgnoreConditions")] = m_ignoreConditions;

	QJsonArray parameters;

	for( auto it = m_parameters.constBegin(), end = m_parameters.constEnd(); it != end; ++it )
	{
		if( isConditionEnabled( it.key() ) )
		{
			QJsonObject parametersObject;
			parametersObject[QStringLiteral("Condition")] = static_cast<int>( it.key() );
			parametersObject[QStringLiteral("Enabled")] = true;
			parametersObject[QStringLiteral("Subject")] = static_cast<int>( subject( it.key() ) );
			parametersObject[QStringLiteral("Argument")] = argument( it.key() );
			parameters.append( parametersObject );
		}
	}

	json[QStringLiteral("Parameters")] = parameters;

	return json;
}
