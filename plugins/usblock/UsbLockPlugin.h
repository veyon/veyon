#pragma once

#include "FeatureProviderInterface.h"
#include <QVariantMap>

enum class UsbLockCommand {
	LockUsb = 0,
	UnlockUsb = 1
};

class VEYON_CORE_EXPORT UsbLockPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	explicit UsbLockPlugin(QObject* parent = nullptr);
	~UsbLockPlugin() override = default;

	Plugin::Uid uid() const override { return Plugin::Uid{QStringLiteral("f3d1e4a5-6789-4bc5-a67b-123456789def")}; }
	QVersionNumber version() const override { return QVersionNumber(1, 0); }
	QString name() const override { return QStringLiteral("usblock"); }
	QString description() const override { return tr("USB Flash Bellek Kilitleme Eklentisi"); }
	QString vendor() const override { return QStringLiteral("Veyon Community"); }
	QString copyright() const override { return QStringLiteral("Efrail"); }
	const FeatureList& featureList() const override { return m_features; }

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

private:
	void setUsbLocked(bool locked);

	const Feature m_lockUsbFeature;
	const Feature m_unlockUsbFeature;
	const FeatureList m_features;
};
