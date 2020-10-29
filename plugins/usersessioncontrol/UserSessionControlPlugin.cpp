/*
 * UserSessionControlPlugin.cpp - implementation of UserSessionControlPlugin class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

#include <QMessageBox>

#include "EnumHelper.h"
#include "PlatformUserFunctions.h"
#include "UserLoginDialog.h"
#include "UserSessionControlPlugin.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"


UserSessionControlPlugin::UserSessionControlPlugin( QObject* parent ) :
	QObject( parent ),
	m_userLoginFeature( QStringLiteral( "UserLogin" ),
						Feature::Action | Feature::Master | Feature::Service,
						Feature::Uid( "7310707d-3918-460d-a949-65bd152cb958" ),
						Feature::Uid(),
						tr( "Log in" ), {},
						tr( "Click this button to log in a specific user on all computers." ),
						QStringLiteral( ":/usersessioncontrol/login-user.png" ) ),
	m_userLogoffFeature( QStringLiteral( "UserLogoff" ),
						 Feature::Action | Feature::Master | Feature::Service,
						 Feature::Uid( "7311d43d-ab53-439e-a03a-8cb25f7ed526" ),
						 Feature::Uid(),
						 tr( "Log off" ), {},
						 tr( "Click this button to log off users from all computers." ),
						 QStringLiteral( ":/usersessioncontrol/logout-user.png" ) ),
	m_features( { m_userLoginFeature, m_userLogoffFeature } )
{
}



bool UserSessionControlPlugin::controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
											  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( operation != Operation::Start )
	{
		return false;
	}

	if( featureUid == m_userLoginFeature.uid() )
	{
		const auto username = arguments.value( EnumHelper::itemName(Arguments::Username) ).toString();
		const auto password = arguments.value( EnumHelper::itemName(Arguments::Password) ).toByteArray();
		if( username.isEmpty() )
		{
			return false;
		}

		sendFeatureMessage( FeatureMessage{ featureUid, FeatureMessage::DefaultCommand }
								.addArgument( Arguments::Username, username )
								.addArgument( Arguments::Password, VeyonCore::cryptoCore().encryptPassword( password ) ),
							computerControlInterfaces );

		return true;
	}

	if( featureUid == m_userLogoffFeature.uid() )
	{
		sendFeatureMessage( FeatureMessage{ featureUid, FeatureMessage::DefaultCommand }, computerControlInterfaces );

		return true;
	}

	return false;
}



bool UserSessionControlPlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
											 const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(master)

	if( confirmFeatureExecution( feature, master.mainWindow() ) == false )
	{
		return false;
	}

	if( feature == m_userLoginFeature )
	{
		UserLoginDialog loginDialog( master.mainWindow() );
		if( loginDialog.exec() )
		{
			return controlFeature( feature.uid(), Operation::Start,
								   {
									   { EnumHelper::itemName(Arguments::Username), loginDialog.username() },
									   { EnumHelper::itemName(Arguments::Password), loginDialog.password().toByteArray() }
								   },
								   computerControlInterfaces );
		}
	}
	else if( feature == m_userLogoffFeature )
	{
		return controlFeature( feature.uid(), Operation::Start, {}, computerControlInterfaces );
	}

	return false;
}



bool UserSessionControlPlugin::handleFeatureMessage( VeyonServerInterface& server,
													 const MessageContext& messageContext,
													 const FeatureMessage& message )
{
	Q_UNUSED(server)
	Q_UNUSED(messageContext)

	if( message.featureUid() == m_userLoginFeature.uid() )
	{
		VeyonCore::platform().userFunctions().prepareLogon( message.argument( Arguments::Username ).toString(),
															VeyonCore::cryptoCore().decryptPassword(
																message.argument( Arguments::Password ).toString() ) );
		return true;
	}

	if( message.featureUid() == m_userLogoffFeature.uid() )
	{
		VeyonCore::platform().userFunctions().logoff();
		return true;
	}

	return false;
}



bool UserSessionControlPlugin::confirmFeatureExecution( const Feature& feature, QWidget* parent )
{
	if( VeyonCore::config().confirmUnsafeActions() == false )
	{
		return true;
	}

	if( feature == m_userLogoffFeature )
	{
		return QMessageBox::question( parent, tr( "Confirm user logoff" ),
									  tr( "Do you really want to log off the selected users?" ) ) ==
				QMessageBox::Yes;
	}

	return true;
}
