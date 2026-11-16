// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Feature.h"
#include "FeatureProviderInterface.h"

class TextMessageFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.TextMessage")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	enum class Argument {
		Text,
		Icon,
		Title
	};
	Q_ENUM(Argument)

	explicit TextMessageFeaturePlugin(QObject* parent = nullptr);
	~TextMessageFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("8ae6668b-9c12-4b29-9bfc-ff89f6604164")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("TextMessage");
	}

	QString description() const override
	{
		return tr("Send a message to a user");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	const FeatureList& featureList() const override;

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool startFeature(VeyonMasterInterface& master, const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

private:
	enum class FeatureCommand {
		ShowTextMessage
	};

	const Feature m_textMessageFeature;
	const FeatureList m_features;
};
