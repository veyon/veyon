/*
 * MonitoringMode.cpp - implementation of MonitoringMode class
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@veyon.io>
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

#include "MonitoringMode.h"
#include "VeyonServerInterface.h"


MonitoringMode::MonitoringMode( QObject* parent ) :
	QObject( parent ),
	m_monitoringModeFeature( Feature::Mode | Feature::Master | Feature::Builtin,
							 Feature::Uid( "edad8259-b4ef-4ca5-90e6-f238d0fda694" ),
							 Feature::Uid(),
							 tr( "Monitoring" ), tr( "Monitoring" ),
							 tr( "This is the default mode and allows you to monitor all computers in one or more rooms." ),
							 QStringLiteral( ":/resources/presentation-none.png" ) ),
	m_features( { m_monitoringModeFeature } )
{
}



bool MonitoringMode::handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
										   ComputerControlInterface::Pointer computerControlInterface )
{
	if( message.featureUid() == m_monitoringModeFeature.uid() &&
			message.command() == PongCommand )
	{
		computerControlInterface->pong();

		return true;
	}

	return SimpleFeatureProvider::handleFeatureMessage( master, message, computerControlInterface );
}



bool MonitoringMode::handleFeatureMessage( VeyonServerInterface& server, const FeatureMessage& message )
{
	if( message.featureUid() == m_monitoringModeFeature.uid() &&
			message.command() == PingCommand )
	{
		return server.sendFeatureMessageReply( message, FeatureMessage( m_monitoringModeFeature.uid(), PongCommand ) );
	}

	return SimpleFeatureProvider::handleFeatureMessage( server, message );

}



void MonitoringMode::ping( ComputerControlInterface::Pointer computerControlInterface )
{
	computerControlInterface->sendFeatureMessage( FeatureMessage( m_monitoringModeFeature.uid(), PingCommand ) );
}
