#pragma once

#include "FeatureProviderInterface.h"
#include <QVariantMap>
#include <QPointer>

class FocusTrackerDialog;

enum class FocusTrackerCommand {
	PollFocus = 0,
	FocusInfo = 1
};

enum class FocusTrackerArgument {
	ActiveWindowTitle = 0
};

class VEYON_CORE_EXPORT FocusTrackerPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	explicit FocusTrackerPlugin(QObject* parent = nullptr);
	~FocusTrackerPlugin() override;

	Plugin::Uid uid() const override { return Plugin::Uid{QStringLiteral("a1b2c3d4-5678-90ab-cdef-123456789012")}; }
	QVersionNumber version() const override { return QVersionNumber(1, 0); }
	QString name() const override { return QStringLiteral("focustracker"); }
	QString description() const override { return tr("Öğrenci Dikkat Takibi (Focus Tracker)"); }
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
	QString getActiveWindowTitle();

	const Feature m_focusTrackerFeature;
	const FeatureList m_features;
	
	QPointer<FocusTrackerDialog> m_dialog;
};
