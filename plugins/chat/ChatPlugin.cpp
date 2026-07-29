#include "ChatPlugin.h"
#include "ChatMasterDialog.h"
#include "ChatStudentDialog.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"
#include "FeatureWorkerManager.h"
#include <QUdpSocket>
#include <QHostInfo>
#include <QMessageBox>

ChatPlugin::ChatPlugin(QObject* parent) :
	QObject(parent),
	m_chatFeature(QStringLiteral("ChatFeature"), Feature::Flag::Action | Feature::Flag::AllComponents,
				  uid(), Feature::Uid(), tr("Sohbet (Chat)"), {}, tr("Öğrencilerle karşılıklı sohbet başlatın."), QStringLiteral(":/core/user-group-new.png")),
	m_features({m_chatFeature})
{
	if (VeyonCore::component() == VeyonCore::Component::Master) {
		m_udpSocket = new QUdpSocket(this);
		m_udpSocket->bind(11411, QUdpSocket::ShareAddress);
		connect(m_udpSocket, &QUdpSocket::readyRead, this, &ChatPlugin::processPendingDatagrams);
	}
}

ChatPlugin::~ChatPlugin()
{
	delete m_masterDialog;
	delete m_studentDialog;
}

void ChatPlugin::processPendingDatagrams()
{
	while (m_udpSocket->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(m_udpSocket->pendingDatagramSize());
		QHostAddress sender;
		m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender);
		
		QString data = QString::fromUtf8(datagram);
		if (data.startsWith(QStringLiteral("HAND:"))) {
			QString host = data.section(QStringLiteral(":"), 1);
			QMessageBox::information(nullptr, tr("El Kaldırma"), tr("%1 (%2) adlı öğrenci soru sormak istiyor!").arg(host, sender.toString()));
		} else if (data.startsWith(QStringLiteral("CHAT:"))) {
			QString host = data.section(QStringLiteral(":"), 1, 1);
			QString msg = data.section(QStringLiteral(":"), 2);
			if (m_masterDialog) {
				m_masterDialog->receiveMessage(host, msg);
			}
		}
	}
}

bool ChatPlugin::controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
								const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments);
	if (featureUid != m_chatFeature.uid()) return false;

	if (operation == Operation::Start) {
		if (!m_masterDialog) {
			m_masterDialog = new ChatMasterDialog(this, computerControlInterfaces);
			m_masterDialog->show();
		} else {
			m_masterDialog->activateWindow();
		}
		
		// Bilgisayarlara Chat penceresini açmaları için komut gönder
		sendFeatureMessage(FeatureMessage{featureUid, FeatureCommand::OpenChat}, computerControlInterfaces);
		return true;
	}
	return false;
}

bool ChatPlugin::handleFeatureMessage(VeyonServerInterface& server,
									  const MessageContext& messageContext,
									  const FeatureMessage& message)
{
	Q_UNUSED(messageContext);
	if (message.featureUid() != m_chatFeature.uid()) return false;

	// Master'dan gelen mesajı Worker'a ilet
	server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(message);
	return true;
}

bool ChatPlugin::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	if (message.featureUid() != m_chatFeature.uid()) return false;
	
	m_worker = &worker;
	auto cmd = message.command<ChatCommand>();

	if (cmd == ChatCommand::OpenChat) {
		if (!m_studentDialog) {
			m_studentDialog = new ChatStudentDialog(this);
		}
		m_studentDialog->show();
		m_studentDialog->activateWindow();
		return true;
	}
	else if (cmd == ChatCommand::MasterMessage) {
		if (!m_studentDialog) {
			m_studentDialog = new ChatStudentDialog(this);
			m_studentDialog->show();
		}
		QString txt = message.argument(ChatArgument::Message).toString();
		m_studentDialog->receiveMessage(txt);
		return true;
	}

	return false;
}

void ChatPlugin::sendMasterMessage(const QString& msg, const ComputerControlInterfaceList& interfaces)
{
	FeatureMessage fmsg(m_chatFeature.uid(), FeatureCommand::MasterMessage);
	fmsg.addArgument(ChatArgument::Message, msg);
	sendFeatureMessage(fmsg, interfaces);
}

void ChatPlugin::sendStudentMessage(const QString& msg)
{
	QUdpSocket udp;
	QByteArray data = QStringLiteral("CHAT:%1:%2").arg(QHostInfo::localHostName(), msg).toUtf8();
	udp.writeDatagram(data, QHostAddress::Broadcast, 11411);
}
