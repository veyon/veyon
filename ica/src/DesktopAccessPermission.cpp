/*
 * DesktopAccessPermission.cpp - DesktopAccessPermission
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtGui/QMessageBox>

#include "DesktopAccessPermission.h"
#include "ItalcConfiguration.h"
#include "ItalcCoreServer.h"
#include "LocalSystem.h"


DesktopAccessPermission::DesktopAccessPermission( AuthenticationMethod authMethod ) :
	m_authenticationMethod( authMethod )
{
}




DesktopAccessPermission::~DesktopAccessPermission()
{
}




bool DesktopAccessPermission::ask( const QString &user, const QString &host )
{
	// look for exceptions
	if( host == QHostAddress( QHostAddress::LocalHost ).toString() )
	{
		return true;
	}

	switch( m_authenticationMethod )
	{
		case KeyAuthentication:
			if( !ItalcCore::config->isPermissionRequiredWithKeyAuthentication() )
			{
				return true;
			}
			break;
		case LogonAuthentication:
			if( !ItalcCore::config->isPermissionRequiredWithLogonAuthentication() )
			{
				return true;
			}
			if( ItalcCore::config->isSameUserConfirmationDisabled() &&
					!user.isEmpty() &&
					LocalSystem::User::loggedOnUser().name() == user )
			{
				return true;
			}
			break;
		default:
			break;
	}

	// okay we got to ask now
	int r = ItalcCoreServer::instance()->slaveManager()->
				execAccessDialog( user, host, ChoiceDefault );
	if( r & ChoiceNo )
	{
		return false;
	}
	if( r & ChoiceYes )
	{
		return true;
	}
	return false;
}

