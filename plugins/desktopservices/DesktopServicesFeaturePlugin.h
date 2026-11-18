// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "QuickConfigurationPluginInterface.h"

#include "ConfigurationPagePluginInterface.h"
#include "DesktopServiceObject.h"
#include "DesktopServicesConfiguration.h"
#include "FeatureProviderInterface.h"

class DesktopServicesFeaturePlugin : public QObject, PluginInterface,
		FeatureProviderInterface,
		ConfigurationPagePluginInterface,
		public QuickConfigurationPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.DesktopServices")
	Q_INTERFACES(QuickConfigurationPluginInterface PluginInterface
					 FeatureProviderInterface
						 ConfigurationPagePluginInterface)
public:
	Q_INVOKABLE bool removePresetFeature(const QString& uid);
	QObject* createQuickConfiguration(Configuration::Object* draft, QObject* parent) override;

	enum class Argument
	{
		Applications,
		WebsiteUrls
	};
	Q_ENUM(Argument)

	explicit DesktopServicesFeaturePlugin(QObject* parent = nullptr);
	~DesktopServicesFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("a54ee018-42bf-4569-90c7-0d8470125ccf")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(2, 0);
	}

	QString name() const override
	{
		return QStringLiteral("DesktopServices");
	}

	QString description() const override
	{
		return tr("Start apps and open websites in user sessions");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	void upgrade(const QVersionNumber& oldVersion) override;

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool startFeature(VeyonMasterInterface& master, const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

	ConfigurationPage* createConfigurationPage() override;

Q_SIGNALS:
	Q_INVOKABLE void acceptStartAppDialog(const QString& app, const QString& saveItemName);
	Q_INVOKABLE void acceptOpenWebsiteDialog(const QString& website, const QString& saveItemName);

private:
	bool eventFilter(QObject* object, QEvent* event) override;

	void updateFeatures();
	void openMenu(const QString& objectName);

	void executeStartAppDialog(VeyonMasterInterface& master,
							   const ComputerControlInterfaceList& computerControlInterfaces);
	void startApp(const QString& app, const QString& saveItemName,
				  VeyonMasterInterface& master, const ComputerControlInterfaceList& computerControlInterfaces);

	void executeOpenWebsiteDialog(VeyonMasterInterface& master,
								  const ComputerControlInterfaceList& computerControlInterfaces);
	void openWebsite(const QString& website, const QString& saveItemName,
					 VeyonMasterInterface& master, const ComputerControlInterfaceList& computerControlInterfaces);

	void runApplicationAsUser(const QString& commandLine);
	bool openWebsite(const QString& urlString);

	void updatePredefinedApplications();
	void updatePredefinedWebsites();

	void updatePredefinedApplicationFeatures();
	void updatePredefinedWebsiteFeatures();

	QString predefinedServicePath(Feature::Uid subFeatureUid) const;

	DesktopServicesConfiguration m_configuration;

	QJsonArray m_predefinedApps;
	QJsonArray m_predefinedWebsites;

	const Feature m_startAppFeature;
	const Feature m_openWebsiteFeature;

	FeatureList m_predefinedAppsFeatures{};
	FeatureList m_predefinedWebsitesFeatures{};

	FeatureList m_features;
};
