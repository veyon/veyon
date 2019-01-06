/*
 * ConfigurationTestController.cpp - class for automated configuration tests
 *
 * Copyright (c) 2016-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "AccessControlProvider.h"
#include "ConfigurationTestController.h"

ConfigurationTestController::ConfigurationTestController( const QStringList &testArguments ) :
    m_testArguments( testArguments )
{
}



int ConfigurationTestController::run()
{
	const QString command = testArgument( 0 ).toLower();

	if( command == QStringLiteral("checkaccess") )
	{
		switch( AccessControlProvider().checkAccess( testArgument( 1 ), testArgument( 2 ),
													 QStringList( testArgument( 3 ) ) ) )
		{
		case AccessControlProvider::AccessAllow: printf( "[TEST]: CheckAccess: ALLOW\n" ); return 0;
		case AccessControlProvider::AccessDeny: printf( "[TEST]: CheckAccess: DENY\n" ); return 0;
		case AccessControlProvider::AccessToBeConfirmed: printf( "[TEST]: CheckAccess: TO BE CONFIRMED\n" ); return 0;
		default: printf( "[TEST]: CheckAccess: FAIL\n" ); return -1;
		}
	}
	else if( command == QStringLiteral("authorizedgroups") )
	{
		if( AccessControlProvider().processAuthorizedGroups( testArgument( 1 ) ) )
		{
			printf( "[TEST]: AuthorizedGroups: ALLOW\n" );
		}
		else
		{
			printf( "[TEST]: AuthorizedGroups: DENY\n" );
		}
		return 0;
	}
	else if( command == QStringLiteral("accesscontrolrules") )
	{
		switch( AccessControlProvider().processAccessControlRules( testArgument( 1 ), testArgument( 2 ),
																   testArgument( 3 ), testArgument( 4 ),
																   QStringList( testArgument( 5 ) ) ) )
		{
		case AccessControlRule::ActionAllow: printf( "[TEST]: AccessControlRules: ALLOW\n" ); return 0;
		case AccessControlRule::ActionDeny: printf( "[TEST]: AccessControlRules: DENY\n" ); return 0;
		case AccessControlRule::ActionNone: printf( "[TEST]: AccessControlRules: NONE\n" ); return 0;
		case AccessControlRule::ActionAskForPermission: printf( "[TEST]: AccessControlRules: ASK FOR PERMISSION\n" ); return 0;
		default: printf( "[TEST]: AccessControlRules: FAIL\n" ); return -1;
		}
	}
	else if( command == QStringLiteral("isaccessdeniedbylocalstate") )
	{
		if( AccessControlProvider().isAccessToLocalComputerDenied() )
		{
			printf( "[TEST]: IsAccessDeniedByLocalState: YES (service will listen on localhost only)\n" );
		}
		else
		{
			printf( "[TEST]: IsAccessDeniedByLocalState: NO (service will listen normally on all interfaces)\n" );
		}
		return 0;
	}
	else
	{
		printf( "TEST COMMANDS:\n\n" );
		printf( "IsAccessDeniedByLocalState\n" );
		printf( "CheckAccess [ACCESSING USER] [ACCESSING COMPUTER] [CONNECTED USER]\n" );
		printf( "AuthorizedGroups [ACCESSING USER]\n" );
		printf( "AccessControlRules [ACCESSING USER] [ACCESSING COMPUTER] [LOCAL USER] [LOCAL COMPUTER] [CONNECTED USER]\n" );
	}

	printf( "\n------------------------------------------------------------------------------\n\n" );

	return -1;
}
