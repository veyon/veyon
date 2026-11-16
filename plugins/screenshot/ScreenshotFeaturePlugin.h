// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Feature.h"
#include "FeatureProviderInterface.h"

class ScreenshotFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.Screenshot")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	explicit ScreenshotFeaturePlugin(QObject* parent = nullptr);
	~ScreenshotFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("ee322521-f4fb-482d-b082-82a79003afa7")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("Screenshot");
	}

	QString description() const override
	{
		return tr("Take screenshots of computers and save them locally.");
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


private:
	const Feature m_screenshotFeature;
	const FeatureList m_features;
};
