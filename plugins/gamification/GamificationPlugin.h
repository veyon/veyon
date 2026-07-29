#pragma once

#include "FeatureProviderInterface.h"
#include <QVariantMap>

class BadgeWidget;

enum class GamificationCommand {
	ShowBadge = 0
};

enum class GamificationArgument {
	BadgeText = 0,
	BadgeColor = 1
};

class VEYON_CORE_EXPORT GamificationPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	explicit GamificationPlugin(QObject* parent = nullptr);
	~GamificationPlugin() override;

	Plugin::Uid uid() const override { return Plugin::Uid{QStringLiteral("e2c9d6e3-1234-4bc5-a67b-123456789abc")}; }
	QVersionNumber version() const override { return QVersionNumber(1, 0); }
	QString name() const override { return QStringLiteral("gamification"); }
	QString description() const override { return tr("Gamification & Badges Plugin"); }
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
	const Feature m_gamificationFeature;
	const FeatureList m_features;

	BadgeWidget* m_badgeWidget = nullptr;
};
