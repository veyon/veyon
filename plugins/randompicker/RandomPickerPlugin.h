#pragma once

#include "FeatureProviderInterface.h"

class LuckyWidget;

enum class RandomPickerCommand {
	YouAreSelected = 0
};

class VEYON_CORE_EXPORT RandomPickerPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	explicit RandomPickerPlugin(QObject* parent = nullptr);
	~RandomPickerPlugin() override;

	Plugin::Uid uid() const override { return Plugin::Uid{QStringLiteral("f1e2d3c4-b5a6-9876-5432-10fedcba9876")}; }
	QVersionNumber version() const override { return QVersionNumber(1, 0); }
	QString name() const override { return QStringLiteral("randompicker"); }
	QString description() const override { return tr("Rastgele Öğrenci Seçici (Şans Çarkı)"); }
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
	const Feature m_pickerFeature;
	const FeatureList m_features;

	LuckyWidget* m_luckyWidget = nullptr;
};
