// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QGuiApplication>
#include <QMutex>

#include "ConfigurationPagePluginInterface.h"
#include "DemoConfiguration.h"
#include "FeatureProviderInterface.h"

class QScreen;

class DemoServer;
class DemoClient;

class DemoFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface, ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.Demo")
	Q_INTERFACES(PluginInterface FeatureProviderInterface ConfigurationPagePluginInterface)
public:
	using Token = CryptoCore::PlaintextPassword;

	enum class Argument {
		DemoAccessToken,
		VncServerPort,
		VncServerPassword,
		DemoServerHost,
		DemoServerPort,
		Viewport,
		ViewportX,
		ViewportY,
		ViewportWidth,
		ViewportHeight,
		VncServerPortOffset
	};
	Q_ENUM(Argument)

	explicit DemoFeaturePlugin(QObject* parent = nullptr);
	~DemoFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("1b08265b-348f-4978-acaa-45d4f6b90bd9")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("Demonstration");
	}

	QString description() const override
	{
		return tr("Give a demonstration by screen broadcasting");
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

	Feature::Uid metaFeature(Feature::Uid featureUid) const override;

	QStringList sensitiveArguments(Feature::Uid featureUid) const override;

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool startFeature(VeyonMasterInterface& master, const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool stopFeature(VeyonMasterInterface& master, const Feature& feature,
					 const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

	bool isFeatureActive(VeyonServerInterface& server, Feature::Uid featureUid) const override;

	ConfigurationPage* createConfigurationPage() override;

private:
	static constexpr auto ScreenSelectionNone = 0;
	static constexpr auto DemoServerControlInterval = 1000;

	void addScreen(QScreen* screen);
	void removeScreen(QScreen* screen);

	void updateFeatures();

	QRect viewportFromScreenSelection() const;

	void setDemoServerActive(bool active);
	void controlDemoServer();
	bool controlDemoClient(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						   const ComputerControlInterfaceList& computerControlInterfaces);

	enum class FeatureCommand {
		StartDemoServer,
		StopDemoServer,
		StartDemoClient,
		StopDemoClient
	};

	const Feature m_demoFeature;
	const Feature m_demoClientFullScreenFeature;
	const Feature m_demoClientWindowFeature;
	const Feature m_shareOwnScreenFullScreenFeature;
	const Feature m_shareOwnScreenWindowFeature;
	const Feature m_shareUserScreenFullScreenFeature;
	const Feature m_shareUserScreenWindowFeature;
	const Feature m_demoServerFeature;
	const FeatureList m_staticFeatures{};
	FeatureList m_features{};

	FeatureList m_screenSelectionFeatures{};
	int m_screenSelection{ScreenSelectionNone};
	QList<QScreen*> m_screens{QGuiApplication::screens()};

	Token m_demoAccessToken;

	DemoConfiguration m_configuration;

	DemoServer* m_demoServer;
	DemoClient* m_demoClient;

	QMutex m_demoServerMutex;
	ComputerControlInterfaceList m_demoServerControlInterfaces{};
	ComputerControlInterfaceList m_demoServerClients{};
	QVariantMap m_demoServerArguments{};
	QTimer m_demoServerControlTimer{this};
	bool m_demoServerActive{false};
};
