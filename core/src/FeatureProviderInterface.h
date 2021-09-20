/*
 * FeatureProviderInterface.h - interface class for feature plugins
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

#pragma once

#include "ComputerControlInterface.h"
#include "EnumHelper.h"
#include "FeatureMessage.h"
#include "Feature.h"
#include "MessageContext.h"
#include "PluginInterface.h"

class VeyonMasterInterface;
class VeyonServerInterface;
class VeyonWorkerInterface;

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT FeatureProviderInterface
{
	Q_GADGET
public:
	enum class Operation
	{
		Initialize,
		Start,
		Stop
	};
	Q_ENUM(Operation)

	virtual ~FeatureProviderInterface() = default;

	/*!
	 * \brief Returns a list of features implemented by the feature class
	 */
	virtual const FeatureList& featureList() const = 0;

	virtual bool hasFeature( Feature::Uid featureUid ) const
	{
		for( const auto& feature : featureList() )
		{
			if( feature.uid() == featureUid )
			{
				return true;
			}
		}

		return false;
	}

	virtual Feature::Uid metaFeature( Feature::Uid ) const
	{
		return {};
	}

	template<class T>
	static QString argToString( T item )
	{
		return EnumHelper::toCamelCaseString( item );
	}

	/*!
	 * \brief Control feature in a generic way based on passed arguments
	 * \param featureUid the UID of the feature to control
	 * \param operation the operation to perform for the feature
	 * \param arguments the arguments specifying what and how to control
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 */
	virtual bool controlFeature( Feature::Uid featureUid,
						Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces ) = 0;

	/*!
	 * \brief Start a feature on master side for given computer control interfaces
	 * \param master a reference to a master instance implementing the VeyonMasterInterface
	 * \param feature the feature to start
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 */
	virtual bool startFeature( VeyonMasterInterface& master,
							   const Feature& feature,
							   const ComputerControlInterfaceList& computerControlInterfaces )
	{
		Q_UNUSED(master)

		return controlFeature( feature.uid(), Operation::Start, {}, computerControlInterfaces );
	}

	/*!
	 * \brief Stops a feature on master side for given computer control interfaces
	 * \param master a reference to a master instance implementing the VeyonMasterInterface
	 * \param feature the feature to stop
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 */
	virtual bool stopFeature( VeyonMasterInterface& master,
							  const Feature& feature,
							  const ComputerControlInterfaceList& computerControlInterfaces )
	{
		Q_UNUSED(master)

		return controlFeature( feature.uid(), Operation::Stop, {}, computerControlInterfaces );
	}

	/*!
	 * \brief Handles a received feature message received on a certain ComputerControlInterface
	 * \param message the message which has been received and needs to be handled
	 * \param computerControlInterfaces the interface over which the message has been received
	 */
	virtual bool handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface ,
							  const FeatureMessage& message )
	{
		Q_UNUSED(computerControlInterface)
		Q_UNUSED(message)

		return false;
	}

	/*!
	 * \brief Handles a received feature message inside server
	 * \param server a reference to a server instance implementing the VeyonServerInterface
	 * \param message the message which has been received and needs to be handled
	 */
	virtual bool handleFeatureMessage( VeyonServerInterface& server,
									   const MessageContext& messageContext,
									   const FeatureMessage& message )
	{
		Q_UNUSED(server)
		Q_UNUSED(messageContext)
		Q_UNUSED(message)

		return false;
	}

	/*!
	 * \brief Handles a received feature message inside worker
	 * \param worker a reference to a worker instance implementing the VeyonWorkerInterface
	 * \param message the message which has been received and needs to be handled
	 */
	virtual bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
	{
		Q_UNUSED(worker)
		Q_UNUSED(message)

		return false;
	}

protected:
	void sendFeatureMessage( const FeatureMessage& message,
							 const ComputerControlInterfaceList& computerControlInterfaces,
							 bool wake = true )
	{
		for( const auto& controlInterface : computerControlInterfaces )
		{
			controlInterface->sendFeatureMessage( message, wake );
		}
	}

};

using FeatureProviderInterfaceList = QList<FeatureProviderInterface *>;

#define FeatureProviderInterface_iid "io.veyon.Veyon.FeatureProviderInterface"

Q_DECLARE_INTERFACE(FeatureProviderInterface, FeatureProviderInterface_iid)
