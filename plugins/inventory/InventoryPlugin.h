#pragma once

#include "FeatureProviderInterface.h"
#include "PluginInterface.h"
#include <QPointer>

#define INVENTORY_FEATURE_UID "6996d99f-7e04-4b57-b2f7-f033c46a9a7a"

enum class InventoryCommand : int {
	GetInventory = 1,
	InventoryData = 2
};

enum class InventoryArgument : int {
	Data = 1
};

class InventoryMasterDialog;

class InventoryPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.Inventory")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	explicit InventoryPlugin(QObject* parent = nullptr);
	~InventoryPlugin() override;

	Plugin::Uid uid() const override { return Plugin::Uid{ QStringLiteral("12345678-abcd-ef00-1234-567890abcdef") }; }
	QVersionNumber version() const override { return QVersionNumber(1, 0); }
	QString name() const override { return QStringLiteral("Inventory"); }
	QString description() const override { return tr("Hardware and Software Inventory"); }
	QString vendor() const override { return QStringLiteral("Veyon Community"); }
	QString copyright() const override { return QStringLiteral("Community"); }

	const FeatureList& featureList() const override { return m_features; }

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						 const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

private:
	const Feature m_inventoryFeature;
	const FeatureList m_features;

	QPointer<InventoryMasterDialog> m_dialog;
};
