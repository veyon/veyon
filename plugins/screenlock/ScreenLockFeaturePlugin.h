// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "FeatureProviderInterface.h"

class LockWidget;

class ScreenLockFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.ScreenLock")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	explicit ScreenLockFeaturePlugin(QObject* parent = nullptr);
	~ScreenLockFeaturePlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("2ad98ccb-e9a5-43ef-8c4c-876ac5efbcb1")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("ScreenLock");
	}

	QString description() const override
	{
		return tr("Lock screen and input devices of a computer");
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

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

private:
	enum class FeatureCommand {
		StartLock,
		StopLock
	};

	const Feature m_screenLockFeature;
	const Feature m_lockInputDevicesFeature;
	const FeatureList m_features;

	LockWidget* m_lockWidget;
};
