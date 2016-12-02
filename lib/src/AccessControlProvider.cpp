/*
 * AccessControlProvider.cpp - implementation of the AccessControlProvider class
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

#include "AccessControlProvider.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"
#include "LocalSystem.h"


AccessControlProvider::AccessControlProvider() :
	m_ldapDirectory()
{
	for( auto encodedRule : ItalcCore::config->accessControlRules() )
	{
		m_accessControlRules.append( AccessControlRule( encodedRule ) );
	}
}



QStringList AccessControlProvider::userGroups()
{
	QStringList groups;

	if( m_ldapDirectory.isBound() )
	{
		groups = m_ldapDirectory.userGroupsNames();
	}
	else
	{
		groups = LocalSystem::userGroups();
	}

	qSort( groups );

	return groups;
}



QStringList AccessControlProvider::computerGroups()
{
	QStringList groups;

	if( m_ldapDirectory.isBound() )
	{
		groups = m_ldapDirectory.computerGroupsNames();
	}

	qSort( groups );

	return groups;
}



QStringList AccessControlProvider::computerPools()
{
	QStringList compoterPoolList;

	if( m_ldapDirectory.isBound() )
	{
		compoterPoolList = m_ldapDirectory.computerPools();
	}

	qSort( compoterPoolList );

	return compoterPoolList;
}
