/*
 * AccessControlRule.h - declaration of class AccessControlRule
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

#ifndef ACCESS_CONTROL_RULE_H
#define ACCESS_CONTROL_RULE_H

#include <QObject>
#include <QVariant>

class AccessControlRule
{
	Q_GADGET
public:
	typedef enum Actions
	{
		ActionNone,
		ActionAllow,
		ActionDeny,
		ActionCount
	} Action;

	Q_ENUM(Action)


	typedef enum Entities
	{
		EntityNone,
		EntityAccessingUser,
		EntityAccessingComputer,
		EntityLocalUser,
		EntityLocalComputer,
		EntityCount
	} Entity;

	Q_ENUM(Entity)


	typedef enum EntityTypes
	{
		EntityTypeNone,
		EntityTypeUser,
		EntityTypeComputer,
		EntityTypeCount
	} EntityType;

	Q_ENUM(EntityType)


	enum Condition
	{
		ConditionNone,
		ConditionMemberOfGroup,
		ConditionMemberOfComputerPool,
		ConditionGroupsInCommon,
		ConditionComputerPoolsInCommon,
		ConditionCount
	} ;

	Q_ENUM(Condition)


	typedef QVariant ConditionArgument;
	typedef QMap<Condition, ConditionArgument> Conditions;


	AccessControlRule();
	AccessControlRule( const AccessControlRule& other );
	AccessControlRule( const QString& encodedData );

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

	Entity entity() const
	{
		return m_entity;
	}

	void setEntity( Entity entity )
	{
		m_entity = entity;
	}

	bool hasCondition( Condition condition ) const
	{
		return m_conditions.contains( condition );
	}

	const Conditions conditions() const
	{
		return m_conditions;
	}

	ConditionArgument conditionArgument( Condition condition ) const
	{
		return m_conditions.value( condition );
	}

	void clearCondition( Condition condition )
	{
		m_conditions.remove( condition );
	}

	void setCondition( Condition condition, ConditionArgument conditionArgument )
	{
		m_conditions[condition] = conditionArgument;
	}

	QString encode() const;

	static EntityType entityType( Entity entity );


private:
	QString m_name;
	QString m_description;
	Action m_action;
	Entity m_entity;
	Conditions m_conditions;

} ;

#endif
