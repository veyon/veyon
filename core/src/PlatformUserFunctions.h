/*
 * PlatformUserFunctions.h - interface class for platform plugins
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "CryptoCore.h"
#include "PlatformPluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class PlatformUserFunctions
{
public:
	using Password = CryptoCore::SecureArray;

	virtual ~PlatformUserFunctions() = default;

	virtual QString fullName( const QString& username ) = 0;

	virtual QStringList userGroups( bool queryDomainGroups ) = 0;
	virtual QStringList groupsOfUser( const QString& username, bool queryDomainGroups ) = 0;

	virtual bool isAnyUserLoggedOn() = 0;
	virtual QString currentUser() = 0;

	virtual bool prepareLogon( const QString& username, const Password& password ) = 0;
	virtual bool performLogon( const QString& username, const Password& password ) = 0;
	virtual void logoff() = 0;

	virtual bool authenticate( const QString& username, const Password& password ) = 0;

};
