/*
 * FeaturePluginInterface.h - interface class for feature plugins
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

#ifndef FEATURE_PLUGIN_INTERFACE_H
#define FEATURE_PLUGIN_INTERFACE_H

#include "ComputerControlInterface.h"
#include "FeatureMessage.h"
#include "Feature.h"
#include "PluginInterface.h"

class FeatureWorkerManager;

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT FeaturePluginInterface
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
	virtual bool startMasterFeature( const Feature& feature,
									 const ComputerControlInterfaceList& computerControlInterfaces,
									 ComputerControlInterface& localComputerControlInterface,
									 QWidget* parent ) = 0;

	/*!
	 * \brief Stops a feature on master side for given computer control interfaces
	 * \param feature the feature to stop
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 * \param parent a pointer to the main window instance
	 */
	virtual bool stopMasterFeature( const Feature& feature,
									const ComputerControlInterfaceList& computerControlInterfaces,
									ComputerControlInterface& localComputerControlInterface,
									QWidget* parent ) = 0;

	/*!
	 * \brief Handles a received feature message inside master
	 * \param message the message which has been received and needs to be handled
	 * \param computerControlInterfaces the interface over which the message has been received
	 */
	virtual bool handleMasterFeatureMessage( const FeatureMessage& message,
											 ComputerControlInterface& computerControlInterface ) = 0;

	/*!
	 * \brief Handles a received feature message inside service
	 * \param message the message which has been received and needs to be handled
	 * \param featureWorkerManager a reference to a FeatureWorkerManager which can be used for starting/stopping workers and communicating with them
	 */
	virtual bool handleServiceFeatureMessage( const FeatureMessage& message,
											  FeatureWorkerManager& featureWorkerManager ) = 0;

	/*!
	 * \brief Handles a received feature message inside worker
	 * \param message the message which has been received and needs to be handled
	 */
	virtual bool handleWorkerFeatureMessage( const FeatureMessage& message ) = 0;

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

typedef QList<FeaturePluginInterface *> FeaturePluginInterfaceList;

#define FeaturePluginInterface_iid "org.veyon.Veyon.Plugins.FeaturePluginInterface"

Q_DECLARE_INTERFACE(FeaturePluginInterface, FeaturePluginInterface_iid)

#endif // FEATURE_PLUGIN_INTERFACE_H
