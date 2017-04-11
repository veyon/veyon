/*
 * UserConfig.h - UserConfig class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include "Configuration/Object.h"

class UserConfig : public Configuration::Object
{
	Q_OBJECT
public:
	UserConfig( Configuration::Store::Backend backend );

#define FOREACH_PERSONAL_CONFIG_PROPERTY(OP)						\
	OP( UserConfig, MasterCore::userConfig, JSONARRAY, checkedNetworkObjects, setCheckedNetworkObjects, "CheckedNetworkObjects", "UI" );	\
	OP( UserConfig, MasterCore::userConfig, INT, monitoringScreenSize, setMonitoringScreenSize, "MonitoringScreenSize", "UI" );	\
	OP( UserConfig, MasterCore::userConfig, INT, defaultRole, setDefaultRole, "DefaultRole", "Authentication" );	\
	OP( UserConfig, MasterCore::userConfig, BOOL, toolButtonIconOnlyMode, setToolButtonIconOnlyMode, "ToolButtonIconOnlyMode", "UI" );	\
	OP( UserConfig, MasterCore::userConfig, BOOL, noToolTips, setNoToolTips, "NoToolTips", "UI" );	\
	OP( UserConfig, MasterCore::userConfig, STRING, windowState, setWindowState, "WindowState", "UI" );	\
	OP( UserConfig, MasterCore::userConfig, STRING, windowGeometry, setWindowGeometry, "WindowGeometry", "UI" );	\

	FOREACH_PERSONAL_CONFIG_PROPERTY(DECLARE_CONFIG_PROPERTY)


public slots:
	void setCheckedNetworkObjects( const QJsonArray& );
	void setMonitoringScreenSize( int );
	void setDefaultRole( int );
	void setToolButtonIconOnlyMode( bool );
	void setNoToolTips( bool );
	void setWindowState( const QString& );
	void setWindowGeometry( const QString& );

} ;

#endif
