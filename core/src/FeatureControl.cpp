/*
 * FeatureControl.cpp - implementation of FeatureControl class
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

#include "FeatureControl.h"
#include "FeatureWorkerManager.h"
#include "VeyonCore.h"
#include "VeyonServerInterface.h"


FeatureControl::FeatureControl( QObject* parent ) :
	QObject( parent ),
	m_featureControlFeature( Feature( QLatin1String( staticMetaObject.className() ),
									  Feature::Service | Feature::Worker | Feature::Builtin,
									  Feature::Uid( "a0a96fba-425d-414a-aaf4-352b76d7c4f3" ),
									  Feature::Uid(),
									  tr( "Feature control" ), {}, {}, {}, {} ) ),
	m_features( { m_featureControlFeature } )
{
}



void FeatureControl::queryActiveFeatures( const ComputerControlInterfaceList& computerControlInterfaces )
{
	sendFeatureMessage( FeatureMessage{ m_featureControlFeature.uid(), QueryActiveFeatures },
						computerControlInterfaces, false );
}



bool FeatureControl::handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
										  const FeatureMessage& message )
{
	if( message.featureUid() == m_featureControlFeature.uid() )
	{
		const auto featureUidStrings = message.argument( Argument::ActiveFeaturesList ).toStringList();

		FeatureUidList activeFeatures{};
		activeFeatures.reserve( featureUidStrings.size() );

		for( const auto& featureUidString : featureUidStrings )
		{
			activeFeatures.append( featureUidString );
		}

		computerControlInterface->setActiveFeatures( activeFeatures );

		return true;
	}

	return false;
}



bool FeatureControl::handleFeatureMessage( VeyonServerInterface& server,
										   const MessageContext& messageContext,
										   const FeatureMessage& message )
{
	if( m_featureControlFeature.uid() == message.featureUid() )
	{
		const auto featureUids = server.featureWorkerManager().runningWorkers();

		QStringList featureUidStrings;
		featureUidStrings.reserve( featureUids.size() );

		for( const auto& featureUid : featureUids )
		{
			featureUidStrings.append( featureUid.toString() );
		}

		FeatureMessage reply( message.featureUid(), message.command() );
		reply.addArgument( Argument::ActiveFeaturesList, featureUidStrings );

		return server.sendFeatureMessageReply( messageContext, reply );
	}

	return false;
}
