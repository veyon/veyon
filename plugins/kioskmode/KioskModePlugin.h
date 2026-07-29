#pragma once

#include "FeatureProviderInterface.h"
#include <QVariantMap>

enum class KioskModeCommand {
	StartKiosk = 0,
	StopKiosk = 1
};

enum class KioskModeArgument {
	Url = 0
};

class VEYON_CORE_EXPORT KioskModePlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	explicit KioskModePlugin(QObject* parent = nullptr);
	~KioskModePlugin() override;

	Plugin::Uid uid() const override { return Plugin::Uid{QStringLiteral("e8a9b2c1-3d4f-5a6b-7c8d-9e0f1a2b3c4d")}; }
	QVersionNumber version() const override { return QVersionNumber(1, 0); }
	QString name() const override { return QStringLiteral("kioskmode"); }
	QString description() const override { return tr("Sınav / Kiosk Modu (Web Filtreleme)"); }
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
	void launchKioskBrowser(const QString& url);
	void killKioskBrowser();

	const Feature m_kioskModeFeature;
	const FeatureList m_features;
};
