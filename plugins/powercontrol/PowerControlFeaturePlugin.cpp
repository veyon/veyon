/*
 * PowerControlFeaturePlugin.cpp - implementation of PowerControlFeaturePlugin class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "Computer.h"
#include "ComputerControlInterface.h"
#include "PowerControl.h"
#include "PowerControlFeaturePlugin.h"


PowerControlFeaturePlugin::PowerControlFeaturePlugin() :
	m_powerOnFeature( Feature::Action | Feature::AllComponents,
					  Feature::Uid( "f483c659-b5e7-4dbc-bd91-2c9403e70ebd" ),
					  tr( "Power on" ), QString(),
					  tr( "Click this button to power on all computers. "
						  "This way you do not have to power on each computer by hand." ),
					  ":/preferences-system-power-management.png" ),
	m_rebootFeature( Feature::Action | Feature::AllComponents,
					 Feature::Uid( "4f7d98f0-395a-4fff-b968-e49b8d0f748c" ),
					 tr( "Reboot" ), QString(),
					 tr( "Click this button to reboot all computers." ),
					 ":/system-reboot.png" ),
	m_powerDownFeature( Feature::Action | Feature::AllComponents,
						Feature::Uid( "6f5a27a0-0e2f-496e-afcc-7aae62eede10" ),
						tr( "Power down" ), QString(),
						tr( "Click this button to power down all computers. "
							"This way you do not have to power down each computer by hand." ),
						":/system-shutdown.png" ),
	m_features()
{
	m_features += m_powerOnFeature;
	m_features += m_rebootFeature;
	m_features += m_powerDownFeature;
}



const FeatureList &PowerControlFeaturePlugin::featureList() const
{
	return m_features;
}



bool PowerControlFeaturePlugin::startMasterFeature( const Feature& feature,
													const ComputerControlInterfaceList& computerControlInterfaces,
													ComputerControlInterface& localComputerControlInterface,
													QWidget* parent )
{
	Q_UNUSED(localComputerControlInterface);

	if( m_features.contains( feature ) == false )
	{
		return false;
	}

	if( feature == m_powerOnFeature )
	{
		for( auto controlInterface : computerControlInterfaces )
		{
			PowerControl::broadcastWOLPacket( controlInterface->computer().macAddress() );
		}
	}
	else
	{
		sendFeatureMessage( FeatureMessage( feature.uid(), FeatureMessage::DefaultCommand ), computerControlInterfaces );
	}

	return true;
}



bool PowerControlFeaturePlugin::stopMasterFeature( const Feature& feature,
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



bool PowerControlFeaturePlugin::handleMasterFeatureMessage( const FeatureMessage& message,
															ComputerControlInterface& computerControlInterface )
{
	Q_UNUSED(message);
	Q_UNUSED(computerControlInterface);

	return false;
}



bool PowerControlFeaturePlugin::handleServiceFeatureMessage( const FeatureMessage& message,
															 FeatureWorkerManager& featureWorkerManager )
{
	if( message.featureUid() == m_powerDownFeature.uid() )
	{
		PowerControl::powerDown();
	}
	else if( message.featureUid() == m_rebootFeature.uid() )
	{
		PowerControl::reboot();
	}
	else
	{
		return false;
	}

	return true;
}



bool PowerControlFeaturePlugin::handleWorkerFeatureMessage( const FeatureMessage& message )
{
	Q_UNUSED(message);

	return false;
}
