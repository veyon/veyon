// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QHash>
#include <QObject>

#include "ComputerControlInterface.h"
#include "Feature.h"
#include "FeatureProviderInterface.h"
#include "Plugin.h"

class QWidget;

class FeatureWorkerManager;

class VEYON_CORE_EXPORT FeatureManager : public QObject
{
	Q_OBJECT
public:
	explicit FeatureManager(QObject* parent = nullptr);

	const FeatureList& features() const
	{
		return m_features;
	}

	const FeatureList& features(Plugin::Uid pluginUid) const;

	const Feature& feature(Feature::Uid featureUid) const;

	const FeatureList& relatedFeatures(Feature::Uid featureUid) const;

	Feature::Uid metaFeatureUid(Feature::Uid featureUid) const;

	Plugin::Uid pluginUid(Feature::Uid featureUid) const;

	QStringList sensitiveArguments(Feature::Uid featureUid) const
	{
		return m_sensitiveArguments.value(featureUid);
	}

	// arguments of a feature message or controlFeature() call for log output: values of sensitive
	// arguments and all values of features unknown here (e.g. from a newer peer) are redacted
	QString redactedArguments(Feature::Uid featureUid, const QVariantMap& arguments) const;


	void controlFeature(Feature::Uid featureUid,
						FeatureProviderInterface::Operation operation,
						const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) const;

	void startFeature(VeyonMasterInterface& master,
					  const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces) const;
	void stopFeature(VeyonMasterInterface& master,
					 const Feature& feature,
					 const ComputerControlInterfaceList& computerControlInterfaces) const;

	void handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface,
							  const FeatureMessage& message) const;
	void handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) const;
	void handleFeatureMessageFromWorker(VeyonServerInterface& server,
										const FeatureMessage& message) const;
	void handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) const;

	void sendAsyncFeatureMessages(VeyonServerInterface& server, const MessageContext& messageContext) const;

	FeatureUidList activeFeatures(VeyonServerInterface& server) const;

private:
	FeatureList m_features;
	FeatureUidList m_disabledFeaturesUids{};
	QHash<Feature::Uid, QStringList> m_sensitiveArguments{};
	const FeatureList m_emptyFeatureList;
	QObjectList m_pluginObjects;
	FeatureProviderInterfaceList m_featurePluginInterfaces;
	const Feature m_dummyFeature;
};
