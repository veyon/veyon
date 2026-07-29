#pragma once

#include "FeatureProviderInterface.h"
#include <QVariantMap>
#include <QUdpSocket>

class QuizCreatorDialog;
class QuizWidget;

enum class QuickQuizCommand {
	SendQuiz = 0
};

enum class QuickQuizArgument {
	QuestionText = 0,
	OptionA = 1,
	OptionB = 2,
	OptionC = 3,
	OptionD = 4
};

class VEYON_CORE_EXPORT QuickQuizPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	explicit QuickQuizPlugin(QObject* parent = nullptr);
	~QuickQuizPlugin() override;

	Plugin::Uid uid() const override { return Plugin::Uid{QStringLiteral("a1b2c3d4-e5f6-1234-5678-9abcdef01234")}; }
	QVersionNumber version() const override { return QVersionNumber(1, 0); }
	QString name() const override { return QStringLiteral("quickquiz"); }
	QString description() const override { return tr("Hızlı Anket ve Quiz Sistemi"); }
	QString vendor() const override { return QStringLiteral("Veyon Community"); }
	QString copyright() const override { return QStringLiteral("Efrail"); }
	const FeatureList& featureList() const override { return m_features; }

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;

	bool handleFeatureMessage(VeyonServerInterface& server,
							  const MessageContext& messageContext,
							  const FeatureMessage& message) override;

	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

	// Used by Student QuizWidget to send answer back via UDP
	void sendAnswerToMaster(const QString& answer);

	// Used by Master Dialog to send quiz
	void sendQuizToStudents(const QString& q, const QString& a, const QString& b, const QString& c, const QString& d, const ComputerControlInterfaceList& interfaces);

private Q_SLOTS:
	void processPendingDatagrams();

private:
	const Feature m_quizFeature;
	const FeatureList m_features;

	QUdpSocket* m_udpSocket = nullptr;
	QuizCreatorDialog* m_creatorDialog = nullptr;
	QuizWidget* m_quizWidget = nullptr;
};
