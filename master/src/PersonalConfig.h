/*
 * PersonalConfig.h - PersonalConfig class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef PERSONAL_CONFIG_H
#define PERSONAL_CONFIG_H

#include "Configuration/Object.h"

class PersonalConfig : public Configuration::Object
{
	Q_OBJECT
public:
	PersonalConfig( Configuration::Store::Backend backend ) :
		Configuration::Object( backend, Configuration::Store::Personal )
	{
	}

#define FOREACH_PERSONAL_CONFIG_PROPERTY(OP)						\
	OP( PersonalConfig, MasterCore::personalConfig, JSONARRAY, checkedNetworkObjects, setCheckedNetworkObjects, "CheckedNetworkObjects", "UI" );	\
	OP( PersonalConfig, MasterCore::personalConfig, INT, clientUpdateInterval, setClientUpdateInterval, "ClientUpdateInterval", "Behaviour" );	\
	OP( PersonalConfig, MasterCore::personalConfig, INT, clientDoubleClickAction, setClientDoubleClickAction, "ClientDoubleClickAction", "Behaviour" );	\
	OP( PersonalConfig, MasterCore::personalConfig, INT, defaultRole, setDefaultRole, "DefaultRole", "Authentication" );	\
	OP( PersonalConfig, MasterCore::personalConfig, INT, toolButtonIconOnlyMode, setToolButtonIconOnlyMode, "ToolButtonIconOnlyMode", "Interface" );	\
	OP( PersonalConfig, MasterCore::personalConfig, INT, noToolTips, setNoToolTips, "NoToolTips", "Interface" );	\

	FOREACH_PERSONAL_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)


public slots:
	void setCheckedNetworkObjects( const QJsonArray& );
	void setClientUpdateInterval( int );
	void setClientDoubleClickAction( int );
	void setDefaultRole( int );
	void setToolButtonIconOnlyMode( int );
	void setNoToolTips( int );

} ;

#endif
