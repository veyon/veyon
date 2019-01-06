/*
 * SimpleFeatureProvider.cpp - implementation of SimpleFeatureProvider class
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "SimpleFeatureProvider.h"


bool SimpleFeatureProvider::startFeature( VeyonMasterInterface& master,
										  const Feature& feature,
										  const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(master)
	Q_UNUSED(feature)
	Q_UNUSED(computerControlInterfaces)

	return false;
}



bool SimpleFeatureProvider::stopFeature( VeyonMasterInterface& master,
										 const Feature& feature,
										 const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(master)
	Q_UNUSED(feature)
	Q_UNUSED(computerControlInterfaces)

	return false;
}



bool SimpleFeatureProvider::handleFeatureMessage( VeyonMasterInterface& master,
												  const FeatureMessage& message,
												  ComputerControlInterface::Pointer computerControlInterface )
{
	Q_UNUSED(master)
	Q_UNUSED(message)
	Q_UNUSED(computerControlInterface)

	return false;
}



bool SimpleFeatureProvider::handleFeatureMessage( VeyonServerInterface& server,
												  const MessageContext& messageContext,
												  const FeatureMessage& message )
{
	Q_UNUSED(server)
	Q_UNUSED(messageContext)
	Q_UNUSED(message)

	return false;
}



bool SimpleFeatureProvider::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker)
	Q_UNUSED(message)

	return false;
}
