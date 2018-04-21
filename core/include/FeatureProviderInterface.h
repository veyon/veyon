/*
 * FeatureProviderInterface.h - interface class for feature plugins
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

#ifndef FEATURE_PROVIDER_INTERFACE_H
#define FEATURE_PROVIDER_INTERFACE_H

#include "ComputerControlInterface.h"
#include "FeatureMessage.h"
#include "Feature.h"
#include "PluginInterface.h"

class VeyonMasterInterface;
class VeyonServerInterface;
class VeyonWorkerInterface;

class FeatureWorkerManager;

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
	 * \param feature the feature to start
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 * \param parent a pointer to the main window instance
	 */
	virtual bool startFeature( VeyonMasterInterface& master,
							   const Feature& feature,
							   const ComputerControlInterfaceList& computerControlInterfaces,
							   QWidget* parent ) = 0;

	/*!
	 * \brief Stops a feature on master side for given computer control interfaces
	 * \param feature the feature to stop
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 * \param parent a pointer to the main window instance
	 */
	virtual bool stopFeature( VeyonMasterInterface& master,
							  const Feature& feature,
							  const ComputerControlInterfaceList& computerControlInterfaces,
							  QWidget* parent ) = 0;

	/*!
	 * \brief Handles a received feature message inside master
	 * \param message the message which has been received and needs to be handled
	 * \param computerControlInterfaces the interface over which the message has been received
	 */
	virtual bool handleFeatureMessage( VeyonMasterInterface& master,
									   const FeatureMessage& message,
									   ComputerControlInterface::Pointer computerControlInterface ) = 0;

	/*!
	 * \brief Handles a received feature message inside server
	 * \param message the message which has been received and needs to be handled
	 * \param featureWorkerManager a reference to a FeatureWorkerManager which can be used for starting/stopping workers and communicating with them
	 */
	virtual bool handleFeatureMessage( VeyonServerInterface& server,
									   const FeatureMessage& message,
									   FeatureWorkerManager& featureWorkerManager ) = 0;

	/*!
	 * \brief Handles a received feature message inside worker
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
