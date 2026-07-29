#pragma once

#include "FeatureProviderInterface.h"
#include <QVariantMap>

class ChatMasterDialog;
class ChatStudentDialog;
class QUdpSocket;

enum class ChatCommand {
	OpenChat = 0,
	MasterMessage = 1
};

enum class ChatArgument {
	Message = 0
};

class VEYON_CORE_EXPORT ChatPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	explicit ChatPlugin(QObject* parent = nullptr);
	~ChatPlugin() override;

	Plugin::Uid uid() const override { return Plugin::Uid{QStringLiteral("e0b8d6e3-1234-4bc5-a67b-123456789abc")}; }
	QVersionNumber version() const override { return QVersionNumber(1, 0); }
	QString name() const override { return QStringLiteral("chat"); }
	QString description() const override { return tr("2-Way Chat Plugin"); }
	QString vendor() const override { return QStringLiteral("Veyon Community"); }
	QString copyright() const override { return QStringLiteral("Efrail"); }
	const FeatureList& featureList() const override { return m_features; }

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

public Q_SLOTS:
	void sendMasterMessage(const QString& msg, const ComputerControlInterfaceList& interfaces);
	void sendStudentMessage(const QString& msg);
	void processPendingDatagrams();

private:
	const Feature m_chatFeature;
	const FeatureList m_features;

	ChatMasterDialog* m_masterDialog = nullptr;
	ChatStudentDialog* m_studentDialog = nullptr;
	VeyonWorkerInterface* m_worker = nullptr;
	QUdpSocket* m_udpSocket = nullptr;
};
