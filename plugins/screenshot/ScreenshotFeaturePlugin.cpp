/*
 * ScreenshotFeaturePlugin.cpp - implementation of ScreenshotFeaturePlugin class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#include <QMessageBox>

#include "ScreenshotFeaturePlugin.h"
#include "ComputerControlInterface.h"
#include "Screenshot.h"


ScreenshotFeaturePlugin::ScreenshotFeaturePlugin() :
	m_screenshotFeature( Feature( Feature::Action | Feature::Master,
								  Feature::Uid( "d5ee3aac-2a87-4d05-b827-0c20344490bd" ),
								  tr( "Screenshot" ), QString(),
								  tr( "Use this function to take a screenshot of selected computers." ),
								  ":/screenshot/camera-photo.png" ) ),
	m_features( { m_screenshotFeature } )
{
}



const FeatureList &ScreenshotFeaturePlugin::featureList() const
{
	return m_features;
}



bool ScreenshotFeaturePlugin::startMasterFeature( const Feature& feature,
												  const ComputerControlInterfaceList& computerControlInterfaces,
												  ComputerControlInterface& localComputerControlInterface,
												  QWidget* parent )
{
	Q_UNUSED(localComputerControlInterface);

	if( feature.uid() == m_screenshotFeature.uid() )
	{
		for( auto controlInterface : computerControlInterfaces )
		{
			Screenshot().take( *controlInterface );

		}

		QMessageBox::information( parent, tr( "Screenshots taken" ),
								  tr( "Screenshot of %1 computer have been taken successfully." ).
								  arg( computerControlInterfaces.count() ) );

		return true;
	}

	return true;
}



bool ScreenshotFeaturePlugin::stopMasterFeature( const Feature& feature,
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



bool ScreenshotFeaturePlugin::handleMasterFeatureMessage( const FeatureMessage& message,
														  ComputerControlInterface& computerControlInterface )
{
	Q_UNUSED(message);
	Q_UNUSED(computerControlInterface);

	return false;
}



bool ScreenshotFeaturePlugin::handleServiceFeatureMessage( const FeatureMessage& message,
														   FeatureWorkerManager& featureWorkerManager )
{
	Q_UNUSED(message);
	Q_UNUSED(featureWorkerManager);

	return false;
}



bool ScreenshotFeaturePlugin::handleWorkerFeatureMessage( const FeatureMessage& message )
{
	Q_UNUSED(message);

	return false;
}
