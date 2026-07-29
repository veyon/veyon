#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include "FeatureProviderInterface.h"

class QuickQuizPlugin;

class QuizCreatorDialog : public QDialog
{
	Q_OBJECT
public:
	explicit QuizCreatorDialog(QuickQuizPlugin* plugin, const ComputerControlInterfaceList& interfaces, QWidget* parent = nullptr);
	~QuizCreatorDialog() override = default;

	void receiveAnswer(const QString& host, const QString& answer);

private Q_SLOTS:
	void sendQuiz();

private:
	void updateResults();

	QuickQuizPlugin* m_plugin;
	ComputerControlInterfaceList m_interfaces;

	QTextEdit* m_questionEdit;
	QLineEdit* m_optA;
	QLineEdit* m_optB;
	QLineEdit* m_optC;
	QLineEdit* m_optD;
	QPushButton* m_sendButton;

	QLabel* m_resA;
	QLabel* m_resB;
	QLabel* m_resC;
	QLabel* m_resD;

	QMap<QString, QString> m_studentAnswers;
};
