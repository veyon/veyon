// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ConfigurationPagePluginInterface.h"
#include "FeatureProviderInterface.h"
#include "InternetBlockConfiguration.h"
#include "InternetBlocker.h"

class InternetBlockFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface, ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.InternetBlock")
	Q_INTERFACES(PluginInterface FeatureProviderInterface ConfigurationPagePluginInterface)
public:
	explicit InternetBlockFeaturePlugin(QObject* parent = nullptr);
	~InternetBlockFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("8f0bb2a5-5d71-4c4e-9d0a-3f5d0a3b6a21")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 0);
	}

	QString name() const override
	{
		return QStringLiteral("InternetBlock");
	}

	QString description() const override
	{
		return tr("Block internet access of computers while keeping the local network reachable");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Veyon Community");
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	bool isFeatureActive(VeyonServerInterface& server, Feature::Uid featureUid) const override;

	ConfigurationPage* createConfigurationPage() override;

	// builds the backend settings from the configuration; additionalAllowedAddress
	// (e.g. the master computer) is added to the allowed IPv4 networks if valid
	static InternetBlocker::Settings settingsFromConfiguration(const InternetBlockConfiguration& configuration,
															   const QString& additionalAllowedAddress = {});

private:
	enum class FeatureCommand {
		Block,
		Unblock
	};

	InternetBlockConfiguration m_configuration;

	const Feature m_internetBlockFeature;
	const FeatureList m_features;

	mutable int m_blockedState = -1; // -1 = unknown, 0 = not blocked, 1 = blocked
};
