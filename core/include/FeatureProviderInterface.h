/*
 * FeatureProviderInterface.h - interface class for feature plugins
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef FEATURE_PROVIDER_INTERFACE_H
#define FEATURE_PROVIDER_INTERFACE_H

#include "ComputerControlInterface.h"
#include "FeatureMessage.h"
#include "Feature.h"
#include "PluginInterface.h"

class VeyonMasterInterface;
class VeyonServerInterface;
class VeyonWorkerInterface;

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT FeatureProviderInterface
{
public:
	/*!
	 * \brief Returns a list of features implemented by the feature class
	 */
	virtual const FeatureList& featureList() const = 0;

	/*!
	 * \brief Start a feature on master side for given computer control interfaces
	 * \param master a reference to a master instance implementing the VeyonMasterInterface
	 * \param feature the feature to start
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 */
	virtual bool startFeature( VeyonMasterInterface& master,
							   const Feature& feature,
							   const ComputerControlInterfaceList& computerControlInterfaces ) = 0;

	/*!
	 * \brief Stops a feature on master side for given computer control interfaces
	 * \param master a reference to a master instance implementing the VeyonMasterInterface
	 * \param feature the feature to stop
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 */
	virtual bool stopFeature( VeyonMasterInterface& master,
							  const Feature& feature,
							  const ComputerControlInterfaceList& computerControlInterfaces ) = 0;

	/*!
	 * \brief Handles a received feature message inside master
	 * \param master a reference to a master instance implementing the VeyonMasterInterface
	 * \param message the message which has been received and needs to be handled
	 * \param computerControlInterfaces the interface over which the message has been received
	 */
	virtual bool handleFeatureMessage( VeyonMasterInterface& master,
									   const FeatureMessage& message,
									   ComputerControlInterface::Pointer computerControlInterface ) = 0;

	/*!
	 * \brief Handles a received feature message inside server
	 * \param server a reference to a server instance implementing the VeyonServerInterface
	 * \param message the message which has been received and needs to be handled
	 */
	virtual bool handleFeatureMessage( VeyonServerInterface& server,
									   const FeatureMessage& message ) = 0;

	/*!
	 * \brief Handles a received feature message inside worker
	 * \param worker a reference to a worker instance implementing the VeyonWorkerInterface
	 * \param message the message which has been received and needs to be handled
	 */
	virtual bool handleFeatureMessage( VeyonWorkerInterface& worker,
									   const FeatureMessage& message ) = 0;

protected:
	bool sendFeatureMessage( const FeatureMessage& message,
							 const ComputerControlInterfaceList& computerControlInterfaces )
	{
		for( auto controlInterface : computerControlInterfaces )
		{
			controlInterface->sendFeatureMessage( message );
		}

		return true;
	}

};

typedef QList<FeatureProviderInterface *> FeatureProviderInterfaceList;

#define FeatureProviderInterface_iid "io.veyon.Veyon.FeatureProviderInterface"

Q_DECLARE_INTERFACE(FeatureProviderInterface, FeatureProviderInterface_iid)

#endif // FEATURE_PROVIDER_INTERFACE_H
