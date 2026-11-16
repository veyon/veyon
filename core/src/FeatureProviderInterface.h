// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ComputerControlInterface.h"
#include "EnumHelper.h"
#include "Feature.h"
#include "FeatureMessage.h"
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
	enum class Operation {
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

	virtual bool hasFeature(Feature::Uid featureUid) const
	{
		for (const auto& feature : featureList())
		{
			if (feature.uid() == featureUid)
			{
				return true;
			}
		}

		return false;
	}

	virtual Feature::Uid metaFeature(Feature::Uid) const
	{
		return {};
	}

	template<class T>
	static QString argToString(T item)
	{
		return EnumHelper::toCamelCaseString(item);
	}

	/*!
	 * \brief Control feature in a generic way based on passed arguments
	 * \param featureUid the UID of the feature to control
	 * \param operation the operation to perform for the feature
	 * \param arguments the arguments specifying what and how to control
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 */
	virtual bool controlFeature(Feature::Uid featureUid,
								Operation operation, const QVariantMap& arguments,
								const ComputerControlInterfaceList& computerControlInterfaces) = 0;

	/*!
	 * \brief Start a feature on master side for given computer control interfaces
	 * \param master a reference to a master instance implementing the VeyonMasterInterface
	 * \param feature the feature to start
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 */
	virtual bool startFeature(VeyonMasterInterface& master,
							  const Feature& feature,
							  const ComputerControlInterfaceList& computerControlInterfaces)
	{
		Q_UNUSED(master)

		return controlFeature(feature.uid(), Operation::Start, {}, computerControlInterfaces);
	}

	/*!
	 * \brief Stops a feature on master side for given computer control interfaces
	 * \param master a reference to a master instance implementing the VeyonMasterInterface
	 * \param feature the feature to stop
	 * \param computerControlInterfaces a list of ComputerControlInterfaces to operate on
	 */
	virtual bool stopFeature(VeyonMasterInterface& master,
							 const Feature& feature,
							 const ComputerControlInterfaceList& computerControlInterfaces)
	{
		Q_UNUSED(master)

		return controlFeature(feature.uid(), Operation::Stop, {}, computerControlInterfaces);
	}

	/*!
	 * \brief Handles a received feature message received on a certain ComputerControlInterface
	 * \param message the message which has been received and needs to be handled
	 * \param computerControlInterfaces the interface over which the message has been received
	 */
	virtual bool handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface,
									  const FeatureMessage& message)
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
	virtual bool handleFeatureMessage(VeyonServerInterface& server,
									  const MessageContext& messageContext,
									  const FeatureMessage& message)
	{
		Q_UNUSED(server)
		Q_UNUSED(messageContext)
		Q_UNUSED(message)

		return false;
	}

	virtual bool handleFeatureMessageFromWorker(VeyonServerInterface& server,
												const FeatureMessage& message)
	{
		Q_UNUSED(server)
		Q_UNUSED(message)

		return false;
	}

	/*!
	 * \brief Send asynchronous messages (e.g. notifications or state updates) to client
	 */
	virtual void sendAsyncFeatureMessages(VeyonServerInterface& server, const MessageContext& messageContext)
	{
		Q_UNUSED(server)
		Q_UNUSED(messageContext)
	}

	/*!
	 * \brief Handles a received feature message inside worker
	 * \param worker a reference to a worker instance implementing the VeyonWorkerInterface
	 * \param message the message which has been received and needs to be handled
	 */
	virtual bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
	{
		Q_UNUSED(worker)
		Q_UNUSED(message)

		return false;
	}

	virtual bool isFeatureActive(VeyonServerInterface& server, Feature::Uid featureUid) const
	{
		Q_UNUSED(featureUid)
		Q_UNUSED(server)

		return false;
	}

protected:
	void sendFeatureMessage(const FeatureMessage& message, const ComputerControlInterfaceList& computerControlInterfaces)
	{
		for (const auto& controlInterface : computerControlInterfaces)
		{
			controlInterface->sendFeatureMessage(message);
		}
	}
};

using FeatureProviderInterfaceList = QList<FeatureProviderInterface*>;

#define FeatureProviderInterface_iid "io.veyon.Veyon.FeatureProviderInterface"

Q_DECLARE_INTERFACE(FeatureProviderInterface, FeatureProviderInterface_iid)
