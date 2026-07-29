#include "QuickQuizPlugin.h"
#include "QuizCreatorDialog.h"
#include "QuizWidget.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"
#include "FeatureWorkerManager.h"
#include "VeyonCore.h"
#include <QHostInfo>

QuickQuizPlugin::QuickQuizPlugin(QObject* parent) :
	QObject(parent),
	m_quizFeature(QStringLiteral("QuickQuiz"), Feature::Flag::Action | Feature::Flag::AllComponents,
				  uid(), Feature::Uid(), tr("Anket Gönder"), {}, tr("Öğrencilere çoktan seçmeli hızlı bir anket veya soru gönderin."), QStringLiteral(":/core/help-about.png")),
	m_features({m_quizFeature})
{
	if (VeyonCore::component() == VeyonCore::Component::Master) {
		m_udpSocket = new QUdpSocket(this);
		m_udpSocket->bind(11412, QUdpSocket::ShareAddress);
		connect(m_udpSocket, &QUdpSocket::readyRead, this, &QuickQuizPlugin::processPendingDatagrams);
	}
}

QuickQuizPlugin::~QuickQuizPlugin()
{
	delete m_creatorDialog;
	delete m_quizWidget;
}

void QuickQuizPlugin::processPendingDatagrams()
{
	while (m_udpSocket->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(m_udpSocket->pendingDatagramSize());
		QHostAddress sender;
		m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender);
		
		QString data = QString::fromUtf8(datagram);
		if (data.startsWith(QStringLiteral("QUIZ:"))) {
			// Format: QUIZ:Answer:Hostname
			QString answer = data.section(QStringLiteral(":"), 1, 1);
			QString host = data.section(QStringLiteral(":"), 2);
			if (m_creatorDialog) {
				m_creatorDialog->receiveAnswer(host, answer);
			}
		}
	}
}

bool QuickQuizPlugin::controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
								const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments);
	if (featureUid != m_quizFeature.uid()) return false;

	if (operation == Operation::Start) {
		if (!m_creatorDialog) {
			m_creatorDialog = new QuizCreatorDialog(this, computerControlInterfaces);
		}
		m_creatorDialog->show();
		m_creatorDialog->activateWindow();
		return true;
	}
	return false;
}

bool QuickQuizPlugin::handleFeatureMessage(VeyonServerInterface& server,
									  const MessageContext& messageContext,
									  const FeatureMessage& message)
{
	Q_UNUSED(messageContext);
	if (message.featureUid() != m_quizFeature.uid()) return false;

	server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(message);
	return true;
}

bool QuickQuizPlugin::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	Q_UNUSED(worker);
	if (message.featureUid() != m_quizFeature.uid()) return false;
	
	auto cmd = message.command<QuickQuizCommand>();
	
	if (cmd == QuickQuizCommand::SendQuiz) {
		QString q = message.argument(QuickQuizArgument::QuestionText).toString();
		QString a = message.argument(QuickQuizArgument::OptionA).toString();
		QString b = message.argument(QuickQuizArgument::OptionB).toString();
		QString c = message.argument(QuickQuizArgument::OptionC).toString();
		QString d = message.argument(QuickQuizArgument::OptionD).toString();

		if (!m_quizWidget) {
			m_quizWidget = new QuizWidget(this);
		}
		m_quizWidget->setQuiz(q, a, b, c, d);
		m_quizWidget->showFullScreen();
		m_quizWidget->activateWindow();
		return true;
	}

	return false;
}

void QuickQuizPlugin::sendQuizToStudents(const QString& q, const QString& a, const QString& b, const QString& c, const QString& d, const ComputerControlInterfaceList& interfaces)
{
	FeatureMessage msg(m_quizFeature.uid(), QuickQuizCommand::SendQuiz);
	msg.addArgument(QuickQuizArgument::QuestionText, q);
	msg.addArgument(QuickQuizArgument::OptionA, a);
	msg.addArgument(QuickQuizArgument::OptionB, b);
	msg.addArgument(QuickQuizArgument::OptionC, c);
	msg.addArgument(QuickQuizArgument::OptionD, d);
	sendFeatureMessage(msg, interfaces);
}

void QuickQuizPlugin::sendAnswerToMaster(const QString& answer)
{
	QUdpSocket udp;
	QByteArray data = QStringLiteral("QUIZ:%1:%2").arg(answer, QHostInfo::localHostName()).toUtf8();
	udp.writeDatagram(data, QHostAddress::Broadcast, 11412);
}
