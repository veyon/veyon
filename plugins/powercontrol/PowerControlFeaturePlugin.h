// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CommandLineIO.h"
#include "CommandLinePluginInterface.h"
#include "Feature.h"
#include "FeatureProviderInterface.h"

class PowerControlFeaturePlugin : public QObject,
								  PluginInterface,
								  CommandLineIO,
								  CommandLinePluginInterface,
								  FeatureProviderInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.PowerControl")
	Q_INTERFACES(PluginInterface CommandLinePluginInterface FeatureProviderInterface)
public:
	enum class Argument {
		ShutdownTimeout
	};
	Q_ENUM(Argument)

	explicit PowerControlFeaturePlugin(QObject* parent = nullptr);
	~PowerControlFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("4122e8ca-b617-4e36-b851-8e050ed2d82e")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 2);
	}

	QString name() const override
	{
		return QStringLiteral("PowerControl");
	}

	QString description() const override
	{
		return tr("Power on/down or reboot a computer");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	QString commandLineModuleName() const override
	{
		return QStringLiteral("power");
	}

	QString commandLineModuleHelp() const override
	{
		return tr("Commands for controlling power status of computers");
	}

	QStringList commands() const override;
	QString commandHelp(const QString& command) const override;

	const FeatureList& featureList() const override;

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool startFeature(VeyonMasterInterface& master, const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

public Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_help(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_on(const QStringList& arguments);

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	bool confirmFeatureExecution(const Feature& feature, bool all, QWidget* parent);
	static bool broadcastWOLPacket(QString macAddress);

	void confirmShutdown();
	void displayShutdownTimeout(int shutdownTimeout);

	QMap<QString, QString> m_commands;

	const Feature m_powerOnFeature;
	const Feature m_rebootFeature;
	const Feature m_powerDownFeature;
	const Feature m_powerDownNowFeature;
	const Feature m_installUpdatesAndPowerDownFeature;
	const Feature m_powerDownConfirmedFeature;
	const Feature m_powerDownDelayedFeature;
	const FeatureList m_features;
};
