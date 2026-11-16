// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QTimer>

#include "FeatureProviderInterface.h"

class FeatureWorkerManager;

class VEYON_CORE_EXPORT DesktopAccessDialog : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	enum class Argument {
		User,
		Host,
		Choice
	};
	Q_ENUM(Argument)

	enum Choice {
		ChoiceNone,
		ChoiceYes,
		ChoiceNo,
		ChoiceAlways,
		ChoiceNever,
		ChoiceCount
	};
	Q_ENUM(Choice)

	explicit DesktopAccessDialog(QObject* parent = nullptr);
	~DesktopAccessDialog() override = default;

	bool isBusy(FeatureWorkerManager* featureWorkerManager) const;

	void exec(FeatureWorkerManager* featureWorkerManager, const QString& user, const QString& host);
	void abort(FeatureWorkerManager* featureWorkerManager);

	Choice choice() const
	{
		return m_choice;
	}

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("13fb9ce8-afbc-4397-93e3-e01c4d8288c9")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("DesktopAccessDialog");
	}

	QString description() const override
	{
		return tr("Desktop access dialog");
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

	bool handleFeatureMessageFromWorker(VeyonServerInterface& server,
										const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

private:
	static Choice requestDesktopAccess(const QString& user, const QString& host);

	enum {
		DialogTimeout = 30000,
		SleepTime = 100
	};

	enum class FeatureCommand {
		RequestDesktopAccess,
		ReportDesktopAccessChoice
	};

	const Feature m_desktopAccessDialogFeature;
	const FeatureList m_features;

	Choice m_choice;

	QTimer m_abortTimer;

Q_SIGNALS:
	void finished();
};
