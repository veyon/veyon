#pragma once

#include "FeatureProviderInterface.h"
#include <QVariantMap>
#include <QTimer>
#include <QStringList>

enum class AppBlockerCommand {
	UpdateBlacklist = 0,
	StopBlocking = 1
};

enum class AppBlockerArgument {
	AppList = 0,
	WhitelistMode = 1
};

class VEYON_CORE_EXPORT AppBlockerPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	explicit AppBlockerPlugin(QObject* parent = nullptr);
	~AppBlockerPlugin() override;

	Plugin::Uid uid() const override { return Plugin::Uid{QStringLiteral("f1e2d3c4-b5a6-9788-1234-56789abcdef0")}; }
	QVersionNumber version() const override { return QVersionNumber(1, 0); }
	QString name() const override { return QStringLiteral("appblocker"); }
	QString description() const override { return tr("Uygulama Engelleyici (Kara Liste)"); }
	QString vendor() const override { return QStringLiteral("Veyon Community"); }
	QString copyright() const override { return QStringLiteral("Efrail"); }
	const FeatureList& featureList() const override { return m_features; }

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;
	void startWorker(VeyonWorkerInterface& worker) override;

private Q_SLOTS:
	void killBlockedApps();

private:
	void loadWorkerBlacklist();
	const Feature m_appBlockerFeature;
	const FeatureList m_features;

	QTimer m_killTimer;
	QStringList m_blockedApps;
	bool m_whitelistModeEnabled = false;
};
