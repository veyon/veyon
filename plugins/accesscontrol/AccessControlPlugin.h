#pragma once

#include "FeatureProviderInterface.h"
#include "ConfigurationPagePluginInterface.h"
#include "AccessControlConfiguration.h"

class AccessControlPlugin : public QObject, public FeatureProviderInterface, public PluginInterface, public ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.AccessControl")
	Q_INTERFACES(PluginInterface FeatureProviderInterface ConfigurationPagePluginInterface)
public:
	explicit AccessControlPlugin( QObject* parent = nullptr );
	~AccessControlPlugin() override;

	Plugin::Uid uid() const override { return Plugin::Uid{ QStringLiteral("c2e81112-9f33-4f1b-b4a1-b4f7ca89547b") }; }
	QVersionNumber version() const override { return QVersionNumber( 1, 0 ); }
	QString name() const override { return QStringLiteral("AccessControl"); }
	QString description() const override { return tr("Internet and Application Access Control"); }
	QString vendor() const override { return QStringLiteral("Veyon Community"); }
	QString copyright() const override { return QStringLiteral("Community"); }

	const FeatureList& featureList() const override { return m_features; }

	ConfigurationPage* createConfigurationPage() override;

	bool controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						 const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

private:
	enum class FeatureCommand
	{
		StartInternetBlock,
		StopInternetBlock,
		StartAppBlock,
		StopAppBlock,
		StartUSBBlock,
		StopUSBBlock
	};

	const Feature m_blockInternetFeature;
	const Feature m_blockAppsFeature;
	const Feature m_blockUSBFeature;
	const FeatureList m_features;

	AccessControlConfiguration m_configuration;
};
