/*
 * FeatureControl.cpp - implementation of FeatureControl class
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@users.sf.net>
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

#include "FeatureControl.h"
#include "FeatureWorkerManager.h"
#include "VeyonCore.h"
#include "VeyonRfbExt.h"


FeatureControl::FeatureControl( QObject* parent ) :
	QObject( parent ),
	m_featureControlFeature( Feature( Feature::Service | Feature::Worker | Feature::Builtin,
									  Feature::Uid( "a0a96fba-425d-414a-aaf4-352b76d7c4f3" ),
									  tr( "Feature control" ), QString(), QString() ) ),
	m_features( { m_featureControlFeature } )
{
}



FeatureControl::~FeatureControl()
{
}



bool FeatureControl::queryActiveFeatures( const ComputerControlInterfaceList& computerControlInterfaces )
{
	return sendFeatureMessage( FeatureMessage( m_featureControlFeature.uid(), QueryActiveFeatures ),
							   computerControlInterfaces );
}



bool FeatureControl::startMasterFeature( const Feature& feature,
										 const ComputerControlInterfaceList& computerControlInterfaces,
										 ComputerControlInterface& localComputerControlInterface,
										 QWidget* parent )
{
	Q_UNUSED(feature);
	Q_UNUSED(computerControlInterfaces);
	Q_UNUSED(localComputerControlInterface);
	Q_UNUSED(parent);

	return false;
}



bool FeatureControl::stopMasterFeature( const Feature& feature,
										const ComputerControlInterfaceList& computerControlInterfaces,
										ComputerControlInterface& localComputerControlInterface,
										QWidget* parent )
{
	Q_UNUSED(feature);
	Q_UNUSED(localComputerControlInterface);
	Q_UNUSED(computerControlInterfaces);
	Q_UNUSED(parent);

	return false;
}



bool FeatureControl::handleMasterFeatureMessage( const FeatureMessage& message,
												 ComputerControlInterface::Pointer computerControlInterface )
{
	if( message.featureUid() == m_featureControlFeature.uid() )
	{
		computerControlInterface->setActiveFeatures( message.argument( ActiveFeatureList ).toStringList() );

		return true;
	}

	return false;
}



bool FeatureControl::handleServiceFeatureMessage( const FeatureMessage& message,
												  FeatureWorkerManager& featureWorkerManager )
{
	Q_UNUSED(featureWorkerManager);

	if( m_featureControlFeature.uid() == message.featureUid() )
	{
		FeatureMessage reply( message.featureUid(), message.command() );
		reply.addArgument( ActiveFeatureList, featureWorkerManager.runningWorkers() );

		char rfbMessageType = rfbVeyonFeatureMessage;
		message.ioDevice()->write( &rfbMessageType, sizeof(rfbMessageType) );
		reply.send( message.ioDevice() );

		return true;
	}

	return false;
}



bool FeatureControl::handleWorkerFeatureMessage( const FeatureMessage& message )
{
	Q_UNUSED(message);

	return false;
}
