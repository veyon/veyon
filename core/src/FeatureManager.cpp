// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FeatureManager.h"
#include "FeatureMessage.h"
#include "FeatureWorkerManager.h"
#include "PluginInterface.h"
#include "PluginManager.h"
#include "VeyonConfiguration.h"
#include "VeyonServerInterface.h"

Q_DECLARE_METATYPE(Feature)
Q_DECLARE_METATYPE(FeatureMessage)

// clazy:excludeall=reserve-candidates

FeatureManager::FeatureManager(QObject* parent) :
	QObject(parent),
	m_features(),
	m_emptyFeatureList(),
	m_pluginObjects(),
	m_dummyFeature()
{
	qRegisterMetaType<Feature>();
	qRegisterMetaType<FeatureMessage>();

	for (const auto& pluginObject : std::as_const(VeyonCore::pluginManager().pluginObjects()))
	{
		auto featurePluginInterface = qobject_cast<FeatureProviderInterface*>(pluginObject);

		if (featurePluginInterface)
		{
			m_pluginObjects += pluginObject;
			m_featurePluginInterfaces += featurePluginInterface;

			m_features += featurePluginInterface->featureList();
		}
	}

	const auto disabledFeatures = VeyonCore::config().disabledFeatures();
	m_disabledFeaturesUids.reserve(disabledFeatures.count());
	for (const auto& disabledFeature : disabledFeatures)
	{
		m_disabledFeaturesUids.append(Plugin::Uid{disabledFeature});
	}
}



const FeatureList& FeatureManager::features(Plugin::Uid pluginUid) const
{
	for (auto pluginObject : m_pluginObjects)
	{
		auto pluginInterface = qobject_cast<PluginInterface*>(pluginObject);
		auto featurePluginInterface = qobject_cast<FeatureProviderInterface*>(pluginObject);

		if (pluginInterface && featurePluginInterface && pluginInterface->uid() == pluginUid)
		{
			return featurePluginInterface->featureList();
		}
	}

	return m_emptyFeatureList;
}



const Feature& FeatureManager::feature(Feature::Uid featureUid) const
{
	for (const auto& featureInterface : m_featurePluginInterfaces)
	{
		for (const auto& feature : featureInterface->featureList())
		{
			if (feature.uid() == featureUid)
			{
				return feature;
			}
		}
	}

	return m_dummyFeature;
}



const FeatureList& FeatureManager::relatedFeatures(Feature::Uid featureUid) const
{
	return features(pluginUid(featureUid));
}



Feature::Uid FeatureManager::metaFeatureUid(Feature::Uid featureUid) const
{
	for (const auto& featureInterface : m_featurePluginInterfaces)
	{
		for (const auto& feature : featureInterface->featureList())
		{
			if (feature.uid() == featureUid)
			{
				return featureInterface->metaFeature(featureUid);
			}
		}
	}

	return {};
}



Plugin::Uid FeatureManager::pluginUid(Feature::Uid featureUid) const
{
	for (auto pluginObject : m_pluginObjects)
	{
		auto pluginInterface = qobject_cast<PluginInterface*>(pluginObject);
		auto featurePluginInterface = qobject_cast<FeatureProviderInterface*>(pluginObject);

		if (pluginInterface && featurePluginInterface &&
			std::find_if(featurePluginInterface->featureList().begin(),
						 featurePluginInterface->featureList().end(),
						 [&featureUid](const Feature& feature) { return feature.uid() == featureUid; }) != featurePluginInterface->featureList().end())
		{
			return pluginInterface->uid();
		}
	}

	return {};
}



void FeatureManager::controlFeature(Feature::Uid featureUid,
									FeatureProviderInterface::Operation operation,
									const QVariantMap& arguments,
									const ComputerControlInterfaceList& computerControlInterfaces) const
{
	for (auto featureInterface : std::as_const(m_featurePluginInterfaces))
	{
		featureInterface->controlFeature(featureUid, operation, arguments, computerControlInterfaces);
	}
}



void FeatureManager::startFeature(VeyonMasterInterface& master,
								  const Feature& feature,
								  const ComputerControlInterfaceList& computerControlInterfaces) const
{
	vDebug() << computerControlInterfaces << feature.name();

	for (auto featureInterface : std::as_const(m_featurePluginInterfaces))
	{
		featureInterface->startFeature(master, feature, computerControlInterfaces);
	}

	if (feature.testFlag(Feature::Flag::Mode))
	{
		for (const auto& controlInterface : computerControlInterfaces)
		{
			controlInterface->setDesignatedModeFeature(feature.uid());
		}
	}
}



void FeatureManager::stopFeature(VeyonMasterInterface& master,
								 const Feature& feature,
								 const ComputerControlInterfaceList& computerControlInterfaces) const
{
	vDebug() << computerControlInterfaces << feature.name();

	for (const auto& featureInterface : std::as_const(m_featurePluginInterfaces))
	{
		featureInterface->stopFeature(master, feature, computerControlInterfaces);
	}

	for (const auto& controlInterface : computerControlInterfaces)
	{
		if (controlInterface->designatedModeFeature() == feature.uid())
		{
			controlInterface->setDesignatedModeFeature(Feature::Uid());
		}
	}
}



void FeatureManager::handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface,
										  const FeatureMessage& message) const
{
	vDebug() << computerControlInterface << message;

	for (const auto& featureInterface : std::as_const(m_featurePluginInterfaces))
	{
		featureInterface->handleFeatureMessage(computerControlInterface, message);
	}
}



void FeatureManager::handleFeatureMessage(VeyonServerInterface& server,
										  const MessageContext& messageContext,
										  const FeatureMessage& message) const
{
	vDebug() << "[SERVER]" << message;

	if (m_disabledFeaturesUids.contains(message.featureUid()))
	{
		vWarning() << "ignoring message as feature" << message.featureUid() << "is disabled by configuration!";
		return;
	}

	for (const auto& featureInterface : std::as_const(m_featurePluginInterfaces))
	{
		featureInterface->handleFeatureMessage(server, messageContext, message);
	}
}



void FeatureManager::handleFeatureMessageFromWorker(VeyonServerInterface& server,
													const FeatureMessage& message) const
{
	vDebug() << "[FROM WORKER]" << message;

	if (m_disabledFeaturesUids.contains(message.featureUid()))
	{
		vWarning() << "ignoring message as feature" << message.featureUid() << "is disabled by configuration!";
		return;
	}

	for (const auto& featureInterface : std::as_const(m_featurePluginInterfaces))
	{
		featureInterface->handleFeatureMessageFromWorker(server, message);
	}
}



void FeatureManager::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) const
{
	vDebug() << "[WORKER]" << message;

	for (const auto& featureInterface : std::as_const(m_featurePluginInterfaces))
	{
		featureInterface->handleFeatureMessage(worker, message);
	}
}



void FeatureManager::sendAsyncFeatureMessages(VeyonServerInterface& server,
											  const MessageContext& messageContext) const
{

	for (const auto& featureInterface : std::as_const(m_featurePluginInterfaces))
	{
		featureInterface->sendAsyncFeatureMessages(server, messageContext);
	}
}



FeatureUidList FeatureManager::activeFeatures(VeyonServerInterface& server) const
{
	FeatureUidList features;

	for (const auto& featureInterface : std::as_const(m_featurePluginInterfaces))
	{
		for (const auto& feature : featureInterface->featureList())
		{
			if (featureInterface->isFeatureActive(server, feature.uid()) ||
				server.featureWorkerManager().isWorkerRunning(feature.uid()))
			{
				features.append(feature.uid());
			}
		}
	}

	return features;
}
