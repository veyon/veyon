#include "QuizCreatorDialog.h"
#include "QuickQuizPlugin.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

QuizCreatorDialog::QuizCreatorDialog(QuickQuizPlugin* plugin, const ComputerControlInterfaceList& interfaces, QWidget* parent)
	: QDialog(parent), m_plugin(plugin), m_interfaces(interfaces)
{
	setWindowTitle(tr("Hızlı Anket Gönder"));
	resize(500, 400);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	mainLayout->addWidget(new QLabel(tr("Soru Metni:")));
	m_questionEdit = new QTextEdit(this);
	mainLayout->addWidget(m_questionEdit);

	m_optA = new QLineEdit(this); m_optA->setPlaceholderText(tr("A Şıkkı"));
	m_optB = new QLineEdit(this); m_optB->setPlaceholderText(tr("B Şıkkı"));
	m_optC = new QLineEdit(this); m_optC->setPlaceholderText(tr("C Şıkkı"));
	m_optD = new QLineEdit(this); m_optD->setPlaceholderText(tr("D Şıkkı"));

	mainLayout->addWidget(m_optA);
	mainLayout->addWidget(m_optB);
	mainLayout->addWidget(m_optC);
	mainLayout->addWidget(m_optD);

	m_sendButton = new QPushButton(tr("Öğrencilere Gönder"), this);
	m_sendButton->setStyleSheet("background-color: #2196F3; color: white; font-weight: bold; padding: 10px;");
	mainLayout->addWidget(m_sendButton);

	QGroupBox* resultsBox = new QGroupBox(tr("Canlı Sonuçlar"), this);
	QVBoxLayout* resultsLayout = new QVBoxLayout(resultsBox);
	m_resA = new QLabel("A: 0", this);
	m_resB = new QLabel("B: 0", this);
	m_resC = new QLabel("C: 0", this);
	m_resD = new QLabel("D: 0", this);
	resultsLayout->addWidget(m_resA);
	resultsLayout->addWidget(m_resB);
	resultsLayout->addWidget(m_resC);
	resultsLayout->addWidget(m_resD);
	mainLayout->addWidget(resultsBox);

	connect(m_sendButton, &QPushButton::clicked, this, &QuizCreatorDialog::sendQuiz);
}

void QuizCreatorDialog::sendQuiz()
{
	m_studentAnswers.clear();
	updateResults();
	m_plugin->sendQuizToStudents(m_questionEdit->toPlainText(), m_optA->text(), m_optB->text(), m_optC->text(), m_optD->text(), m_interfaces);
}

void QuizCreatorDialog::receiveAnswer(const QString& host, const QString& answer)
{
	m_studentAnswers[host] = answer;
	updateResults();
}

void QuizCreatorDialog::updateResults()
{
	int countA = 0, countB = 0, countC = 0, countD = 0;
	for (const QString& ans : m_studentAnswers.values()) {
		if (ans == "A") countA++;
		else if (ans == "B") countB++;
		else if (ans == "C") countC++;
		else if (ans == "D") countD++;
	}
	m_resA->setText(QString("A: %1 kişi").arg(countA));
	m_resB->setText(QString("B: %1 kişi").arg(countB));
	m_resC->setText(QString("C: %1 kişi").arg(countC));
	m_resD->setText(QString("D: %1 kişi").arg(countD));
}
