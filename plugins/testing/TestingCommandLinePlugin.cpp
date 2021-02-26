/*
 * TestingCommandLinePlugin.cpp - implementation of TestingCommandLinePlugin class
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

#include "CommandLineIO.h"
#include "AccessControlProvider.h"
#include "TestingCommandLinePlugin.h"


TestingCommandLinePlugin::TestingCommandLinePlugin( QObject* parent ) :
	QObject( parent ),
	m_commands( {
{ QStringLiteral("checkaccess"), QStringLiteral( "check access with arguments [ACCESSING USER] [ACCESSING COMPUTER] [CONNECTED USER]" ) },
{ QStringLiteral("authorizedgroups"), QStringLiteral( "check if specified user is in authorized groups [ACCESSING USER]" ) },
{ QStringLiteral("accesscontrolrules"), QStringLiteral( "process access control rules with arguments [ACCESSING USER] [ACCESSING COMPUTER] [LOCAL USER] [LOCAL COMPUTER] [CONNECTED USER]" ) },
{ QStringLiteral("isaccessdeniedbylocalstate"), QStringLiteral( "check if access would be denied by local state") },
				} )
{
}



QStringList TestingCommandLinePlugin::commands() const
{
	return m_commands.keys();
}



QString TestingCommandLinePlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult TestingCommandLinePlugin::handle_checkaccess( const QStringList& arguments )
{

	switch( AccessControlProvider().checkAccess( arguments.value( 0 ), arguments.value( 1 ), { arguments.value( 2 ) } ) )
	{
	case AccessControlProvider::Access::Allow: printf( "[TEST]: CheckAccess: ALLOW\n" ); return Successful;
	case AccessControlProvider::Access::Deny: printf( "[TEST]: CheckAccess: DENY\n" ); return Successful;
	case AccessControlProvider::Access::ToBeConfirmed: printf( "[TEST]: CheckAccess: TO BE CONFIRMED\n" ); return Successful;
	}

	printf( "[TEST]: CheckAccess: FAIL\n" );
	return Failed;
}



CommandLinePluginInterface::RunResult TestingCommandLinePlugin::handle_authorizedgroups( const QStringList& arguments )
{
	if( AccessControlProvider().processAuthorizedGroups( arguments.value( 0 ) ) )
	{
		printf( "[TEST]: AuthorizedGroups: ALLOW\n" );
	}
	else
	{
		printf( "[TEST]: AuthorizedGroups: DENY\n" );
	}
	return Successful;
}



CommandLinePluginInterface::RunResult TestingCommandLinePlugin::handle_accesscontrolrules( const QStringList& arguments )
{
	switch( AccessControlProvider().processAccessControlRules( arguments.value( 0 ), arguments.value( 1 ),
															   arguments.value( 2 ), arguments.value( 3 ),
															   QStringList( arguments.value( 4 ) ) ) )
	{
	case AccessControlRule::Action::Allow: printf( "[TEST]: AccessControlRules: ALLOW\n" ); return Successful;
	case AccessControlRule::Action::Deny: printf( "[TEST]: AccessControlRules: DENY\n" ); return Successful;
	case AccessControlRule::Action::None: printf( "[TEST]: AccessControlRules: NONE\n" ); return Successful;
	case AccessControlRule::Action::AskForPermission: printf( "[TEST]: AccessControlRules: ASK FOR PERMISSION\n" ); return Successful;
	}

	printf( "[TEST]: AccessControlRules: FAIL\n" );
	return Failed;
}



CommandLinePluginInterface::RunResult TestingCommandLinePlugin::handle_isaccessdeniedbylocalstate( const QStringList& arguments )
{
	Q_UNUSED(arguments)

	if( AccessControlProvider().isAccessToLocalComputerDenied() )
	{
		printf( "[TEST]: IsAccessDeniedByLocalState: YES (server will listen on localhost only)\n" );
	}
	else
	{
		printf( "[TEST]: IsAccessDeniedByLocalState: NO (server will listen normally on all interfaces)\n" );
	}

	return Successful;
}
