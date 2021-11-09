/*
 * MonitoringMode.cpp - implementation of MonitoringMode class
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

#include <QtConcurrent>
#include <QGuiApplication>
#include <QScreen>

#include "MonitoringMode.h"
#include "PlatformSessionFunctions.h"
#include "PlatformUserFunctions.h"
#include "VeyonServerInterface.h"


MonitoringMode::MonitoringMode( QObject* parent ) :
	QObject( parent ),
	m_monitoringModeFeature( QLatin1String( staticMetaObject.className() ),
							 Feature::Flag::Mode | Feature::Flag::Master | Feature::Flag::Builtin,
							 Feature::Uid( "edad8259-b4ef-4ca5-90e6-f238d0fda694" ),
							 Feature::Uid(),
							 tr( "Monitoring" ), tr( "Monitoring" ),
							 tr( "This mode allows you to monitor all computers at one or more locations." ),
							 QStringLiteral( ":/core/presentation-none.png" ) ),
	m_queryLoggedOnUserInfoFeature( QStringLiteral("UserSessionInfo"),
									Feature::Flag::Session | Feature::Flag::Service | Feature::Flag::Worker | Feature::Flag::Builtin,
									Feature::Uid( "79a5e74d-50bd-4aab-8012-0e70dc08cc72" ),
									Feature::Uid(), {}, {}, {} ),
	m_queryDisplaysFeature( QStringLiteral("QueryDisplays"),
						   Feature::Flag::Meta,
						   Feature::Uid("d5bbc486-7bc5-4c36-a9a8-1566c8b0091a"),
						   Feature::Uid(), tr("Query properties of attached displays"), {}, {} ),
	m_features( { m_monitoringModeFeature, m_queryLoggedOnUserInfoFeature, m_queryDisplaysFeature } )
{
}



void MonitoringMode::queryLoggedOnUserInfo( const ComputerControlInterfaceList& computerControlInterfaces )
{
	sendFeatureMessage( FeatureMessage{ m_queryLoggedOnUserInfoFeature.uid(), FeatureMessage::DefaultCommand },
						computerControlInterfaces, false );
}



void MonitoringMode::queryDisplays(const ComputerControlInterfaceList& computerControlInterfaces)
{
	sendFeatureMessage( FeatureMessage{m_queryDisplaysFeature.uid(), FeatureMessage::DefaultCommand},
						computerControlInterfaces );
}



bool MonitoringMode::handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
										  const FeatureMessage& message )
{
	if( message.featureUid() == m_queryLoggedOnUserInfoFeature.uid() )
	{
		computerControlInterface->setUserInformation( message.argument( Argument::UserLoginName ).toString(),
													  message.argument( Argument::UserFullName ).toString(),
													  message.argument( Argument::UserSessionId ).toInt() );

		return true;
	}

	if( message.featureUid() == m_queryDisplaysFeature.uid() )
	{
		const auto displayInfoList = message.argument(Argument::DisplayInfoList).toList();

		ComputerControlInterface::DisplayList displays;
		displays.reserve(displayInfoList.size());

		for(int i = 0; i < displayInfoList.size(); ++i)
		{
			const auto displayInfo = displayInfoList.at(i).toMap();
			ComputerControlInterface::DisplayProperties displayProperties;
			displayProperties.index = i + 1;
			displayProperties.name = displayInfo.value(QStringLiteral("name")).toString();
			displayProperties.geometry = displayInfo.value(QStringLiteral("geometry")).toRect();
			displays.append(displayProperties);
		}

		computerControlInterface->setDisplays(displays);
	}

	return false;
}



bool MonitoringMode::handleFeatureMessage( VeyonServerInterface& server,
										   const MessageContext& messageContext,
										   const FeatureMessage& message )
{
	if( m_queryLoggedOnUserInfoFeature.uid() == message.featureUid() )
	{
		FeatureMessage reply( message.featureUid(), message.command() );

		m_userDataLock.lockForRead();
		if( m_userLoginName.isEmpty() )
		{
			queryUserInformation();
			reply.addArgument( Argument::UserLoginName, QString() );
			reply.addArgument( Argument::UserFullName, QString() );
			reply.addArgument( Argument::UserSessionId, -1 );
		}
		else
		{
			reply.addArgument( Argument::UserLoginName, m_userLoginName );
			reply.addArgument( Argument::UserFullName, m_userFullName );
			reply.addArgument( Argument::UserSessionId, m_userSessionId );
		}
		m_userDataLock.unlock();

		return server.sendFeatureMessageReply( messageContext, reply );
	}

	if( message.featureUid() == m_queryDisplaysFeature.uid() )
	{
		const auto screens = QGuiApplication::screens();

		QVariantList displayInfoList;
		displayInfoList.reserve(screens.size());

		int index = 1;
		for(const auto* screen : screens)
		{
			QVariantMap displayInfo;
			displayInfo[QStringLiteral("index")] = index;
			displayInfo[QStringLiteral("name")] = screen->name();
			displayInfo[QStringLiteral("geometry")] = screen->geometry();
			displayInfoList.append(displayInfo);
			++index;
		}

		return server.sendFeatureMessageReply( messageContext,
											   FeatureMessage(m_queryDisplaysFeature.uid())
												   .addArgument(Argument::DisplayInfoList, displayInfoList) );
	}

	return false;
}



void MonitoringMode::queryUserInformation()
{
	// asynchronously query information about logged on user (which might block
	// due to domain controller queries and timeouts etc.)
	(void) QtConcurrent::run( [=]() {
		if( VeyonCore::platform().sessionFunctions().currentSessionHasUser() )
		{
			const auto userLoginName = VeyonCore::platform().userFunctions().currentUser();
			const auto userFullName = VeyonCore::platform().userFunctions().fullName( userLoginName );
			const auto userSessionId = VeyonCore::sessionId();
			m_userDataLock.lockForWrite();
			m_userLoginName = userLoginName;
			m_userFullName = userFullName;
			m_userSessionId = userSessionId;
			m_userDataLock.unlock();
		}
	} );
}
