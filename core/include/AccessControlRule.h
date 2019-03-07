/*
 * AccessControlRule.h - declaration of class AccessControlRule
 *
 * Copyright (c) 2016-2019 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <QObject>
#include <QJsonObject>
#include <QVariant>

#include "VeyonCore.h"

class VEYON_CORE_EXPORT AccessControlRule
{
	Q_GADGET
public:
	typedef enum Actions
	{
		ActionNone,
		ActionAllow,
		ActionDeny,
		ActionAskForPermission,
		ActionCount
	} Action;

	Q_ENUM(Action)

	enum Condition
	{
		ConditionNone,
		ConditionMemberOfUserGroup,
		ConditionGroupsInCommon,
		ConditionLocatedAt,
		ConditionSameLocation,
		ConditionAccessFromLocalHost,
		ConditionAccessFromLocalUser,
		ConditionAccessFromAlreadyConnectedUser,
		ConditionNoUserLoggedOn,
		ConditionCount
	} ;

	Q_ENUM(Condition)

	typedef enum Subjects
	{
		SubjectNone,
		SubjectAccessingUser,
		SubjectAccessingComputer,
		SubjectLocalUser,
		SubjectLocalComputer,
		SubjectCount
	} Subject;

	Q_ENUM(Subject)

	typedef QString ConditionArgument;

	struct ConditionParameters
	{
		bool enabled;
		Subject subject;
		ConditionArgument argument;
	};

	typedef QMap<Condition, ConditionParameters> ConditionParameterMap;


	AccessControlRule();
	AccessControlRule( const AccessControlRule& other );
	AccessControlRule( const QJsonValue& jsonValue );

	~AccessControlRule() {}

	AccessControlRule& operator=( const AccessControlRule& other );

	const QString& name() const
	{
		return m_name;
	}

	void setName( const QString& name )
	{
		m_name = name;
	}

	const QString& description() const
	{
		return m_description;
	}

	void setDescription( const QString& description )
	{
		m_description = description;
	}

	Action action() const
	{
		return m_action;
	}

	void setAction( Action action )
	{
		m_action = action;
	}

	const ConditionParameterMap& parameters() const
	{
		return m_parameters;
	}

	Subject subject( Condition condition ) const
	{
		return m_parameters.value( condition ).subject;
	}

	void setSubject( Condition condition, Subject subject )
	{
		m_parameters[condition].subject = subject;
	}

	bool areConditionsIgnored() const
	{
		return m_ignoreConditions;
	}

	void setConditionsIgnored( bool ignored )
	{
		m_ignoreConditions = ignored;
	}

	bool areConditionsInverted() const
	{
		return m_invertConditions;
	}

	void setConditionsInverted( bool inverted )
	{
		m_invertConditions = inverted;
	}

	bool isConditionEnabled( Condition condition ) const
	{
		return m_parameters.value( condition ).enabled;
	}

	void setConditionEnabled( Condition condition, bool enabled )
	{
		m_parameters[condition].enabled = enabled;
	}

	ConditionArgument argument( Condition condition ) const
	{
		return m_parameters.value( condition ).argument;
	}

	void clearParameters()
	{
		m_parameters.clear();
	}

	void setArgument( Condition condition, const ConditionArgument& conditionArgument )
	{
		m_parameters[condition].argument = conditionArgument;
	}

	QJsonObject toJson() const;


private:
	QString m_name;
	QString m_description;
	Action m_action;
	ConditionParameterMap m_parameters;
	bool m_invertConditions;
	bool m_ignoreConditions;

} ;
