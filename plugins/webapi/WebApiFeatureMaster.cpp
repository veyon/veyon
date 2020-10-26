/*
 * WebApiFeatureMaster.cpp - implementation of WebApiFeatureMaster class
 *
 * Copyright (c) 2020 Tobias Junghans <tobydox@veyon.io>
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

#include "WebApiFeatureMaster.h"


WebApiFeatureMaster::WebApiFeatureMaster( QObject* parent ) :
	VeyonMasterInterface( parent ),
	m_featureManager( this ),
	m_sessionControlInterface( Computer{}, this )
{
}



QWidget* WebApiFeatureMaster::mainWindow()
{
	vCritical() << "non-existent main window requested - expect crash!";
	return nullptr;
}



Configuration::Object* WebApiFeatureMaster::userConfigurationObject()
{
	vCritical() << "returning dummy configuration";
	return &m_dummyConfiguration;
}



void WebApiFeatureMaster::reloadSubFeatures()
{
}



ComputerControlInterface& WebApiFeatureMaster::localSessionControlInterface()
{
	return m_sessionControlInterface;
}



void WebApiFeatureMaster::registerMessageHandler( ComputerControlInterface* controlInterface )
{
	connect( controlInterface, &ComputerControlInterface::featureMessageReceived, this,
			 [this]( const FeatureMessage& featureMessage, ComputerControlInterface::Pointer computerControlInterface ) {
				 m_featureManager.handleFeatureMessage( *this, featureMessage, computerControlInterface );
			 } );

}



FeatureList WebApiFeatureMaster::availableFeatures() const
{
	return m_featureManager.features();
}



bool WebApiFeatureMaster::hasFeature( Feature::Uid featureUid ) const
{
	return m_featureManager.feature( featureUid ).isValid();
}



void WebApiFeatureMaster::startFeature( Feature::Uid featureUid, ComputerControlInterface* controlInterface )
{
	m_featureManager.startFeature( *this, m_featureManager.feature( featureUid ),
								   { ComputerControlInterface::Pointer{ controlInterface,
																	  []( ComputerControlInterface* ) {} } } );
}



void WebApiFeatureMaster::stopFeature( Feature::Uid featureUid, ComputerControlInterface* controlInterface )
{
	m_featureManager.stopFeature( *this, m_featureManager.feature( featureUid ),
								  { ComputerControlInterface::Pointer{ controlInterface,
																	 []( ComputerControlInterface* ) {} } } );
}
