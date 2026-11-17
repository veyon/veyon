// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QReadWriteLock>

#include "FeatureProviderInterface.h"

class UserSessionControlPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.UserSessionControl")
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	enum class Argument {
		Username,
		Password
	};
	Q_ENUM(Argument)

	explicit UserSessionControlPlugin(QObject* parent = nullptr);
	~UserSessionControlPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("80580500-2e59-4297-9e35-e53959b028cd")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 2);
	}

	QString name() const override
	{
		return QStringLiteral("UserSessionControl");
	}

	QString description() const override
	{
		return tr("User session control");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	QStringList sensitiveArguments(Feature::Uid featureUid) const override;

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool startFeature(VeyonMasterInterface& master, const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

private:
	bool confirmFeatureExecution(const Feature& feature, bool all, QWidget* parent);

	const Feature m_userLoginFeature;
	const Feature m_userLogoffFeature;
	const FeatureList m_features;
};
