// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "FeatureProviderInterface.h"

class QSystemTrayIcon;
class FeatureWorkerManager;

class VEYON_CORE_EXPORT SystemTrayIcon : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	enum class Argument {
		ToolTipText,
		MessageTitle,
		MessageText,
		OverlayIconUrl
	};
	Q_ENUM(Argument)

	explicit SystemTrayIcon(QObject* parent = nullptr);
	~SystemTrayIcon() override = default;

	void setToolTip(const QString& toolTipText,
					FeatureWorkerManager& featureWorkerManager);

	void showMessage(const QString& messageTitle,
					 const QString& messageText,
					 FeatureWorkerManager& featureWorkerManager);

	void showMessage(const ComputerControlInterfaceList& computerControlInterfaces,
					 const QString& messageTitle, const QString& messageText);

	void setOverlay(const ComputerControlInterfaceList& computerControlInterfaces, const QString& iconUrl);
	void clearOverlay(const ComputerControlInterfaceList& computerControlInterfaces)
	{
		setOverlay(computerControlInterfaces, {});
	}

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("3cb1adb1-6b4d-4934-a641-db767df83eea")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("SystemTrayIcon");
	}

	QString description() const override
	{
		return tr("System tray icon");
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

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override
	{
		Q_UNUSED(featureUid)
		Q_UNUSED(operation)
		Q_UNUSED(arguments)
		Q_UNUSED(computerControlInterfaces)

		return false;
	}

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

private:
	void updateIcon(const QString& overlayIconUrl = {});

	static constexpr int MessageBoxAutoCloseInterval = 10000;

	enum class FeatureCommand {
		SetToolTip,
		ShowMessage,
		SetOverlayIcon
	};

	const Feature m_systemTrayIconFeature;
	const FeatureList m_features = {m_systemTrayIconFeature};

	QSystemTrayIcon* m_systemTrayIcon = nullptr;
	bool m_hidden = false;
};
