// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CommandLinePluginInterface.h"
#include "Computer.h"
#include "FeatureProviderInterface.h"

class VncView;

class RemoteAccessFeaturePlugin : public QObject, CommandLinePluginInterface, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.RemoteAccess")
	Q_INTERFACES(PluginInterface FeatureProviderInterface CommandLinePluginInterface)
public:
	enum class Argument {
		HostName,
		ClipboardText,
		ClipboardImage
	};
	Q_ENUM(Argument)

	explicit RemoteAccessFeaturePlugin(QObject* parent = nullptr);
	~RemoteAccessFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("387a0c43-1355-4ff6-9e1f-d098e9ce5127")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 1);
	}

	QString name() const override
	{
		return QStringLiteral("RemoteAccess");
	}

	QString description() const override
	{
		return tr("Remote view or control a computer");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	const FeatureList& featureList() const override;

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool startFeature(VeyonMasterInterface& master, const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

	void sendAsyncFeatureMessages(VeyonServerInterface& server, const MessageContext& messageContext) override;

	QString commandLineModuleName() const override
	{
		return QStringLiteral("remoteaccess");
	}

	QString commandLineModuleHelp() const override
	{
		return description();
	}

	QStringList commands() const override;

	QString commandHelp(const QString& command) const override;

private Q_SLOTS:
	CommandLinePluginInterface::RunResult handle_view(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_control(const QStringList& arguments);
	CommandLinePluginInterface::RunResult handle_help(const QStringList& arguments);

private:
	static const char* clipboardDataVersionProperty()
	{
		return "clipboardDataVersion";
	}

	static const char* clipboardImageFormat()
	{
		return "PNG";
	}

	bool remoteViewEnabled() const;
	bool remoteControlEnabled() const;
	bool initAuthentication();
	bool remoteAccess(const QString& hostAddress, bool viewOnly);
	void createRemoteAccessWindow(const ComputerControlInterface::Pointer& computerControlInterface, bool viewOnly);

	void storeClipboardData(FeatureMessage* message, const QString& text, const QImage& image);
	void loadClipboardData(const FeatureMessage& message);
	void sendClipboardData(ComputerControlInterface::Pointer computerControlInterface);

	void updateClipboardData();

	const Feature m_remoteViewFeature;
	const Feature m_remoteControlFeature;
	const Feature m_clipboardExchangeFeature;
	const FeatureList m_features;

	QMap<QString, QString> m_commands;

	bool m_clipboardSynchronizationDisabled;
	QMutex m_clipboardDataMutex;
	int m_clipboardDataVersion{0};
	QString m_clipboardText;
	QImage m_clipboardImage;

	QList<QPair<QPointer<QObject>, ComputerControlInterface::Pointer>> m_vncViews{};

};
