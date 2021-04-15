/*
 * LinuxUserFunctions.h - declaration of LinuxUserFunctions class
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

#include "LogonHelper.h"
#include "PlatformUserFunctions.h"

#include <pwd.h>

// clazy:excludeall=copyable-polymorphic

class LinuxUserFunctions : public PlatformUserFunctions
{
public:
	QString fullName( const QString& username ) override;

	QStringList userGroups( bool queryDomainGroups ) override;
	QStringList groupsOfUser( const QString& username, bool queryDomainGroups ) override;

	bool isAnyUserLoggedOn() override;
	QString currentUser() override;

	bool prepareLogon( const QString& username, const Password& password ) override;
	bool performLogon( const QString& username, const Password& password ) override;
	void logoff() override;

	bool authenticate( const QString& username, const Password& password ) override;

	static uid_t userIdFromName( const QString& username );

private:
	static constexpr auto AuthHelperTimeout = 10000;

	LogonHelper m_logonHelper{};

};
