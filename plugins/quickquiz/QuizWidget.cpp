#include "QuizWidget.h"
#include "QuickQuizPlugin.h"
#include <QVBoxLayout>
#include <QPainter>

QuizWidget::QuizWidget(QuickQuizPlugin* plugin, QWidget* parent)
	: QWidget(parent), m_plugin(plugin)
{
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setAlignment(Qt::AlignCenter);

	QWidget* centerBox = new QWidget(this);
	centerBox->setStyleSheet("background-color: rgba(30, 30, 40, 240); border-radius: 15px; color: white;");
	centerBox->setFixedSize(600, 400);

	QVBoxLayout* boxLayout = new QVBoxLayout(centerBox);
	m_questionLabel = new QLabel(this);
	m_questionLabel->setWordWrap(true);
	m_questionLabel->setStyleSheet("font-size: 24px; font-weight: bold; margin: 20px;");
	boxLayout->addWidget(m_questionLabel);

	auto createBtn = [this](const QString& ans) {
		QPushButton* btn = new QPushButton(this);
		btn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-size: 18px; padding: 15px; border-radius: 8px; margin: 5px; } QPushButton:hover { background-color: #45a049; }");
		connect(btn, &QPushButton::clicked, this, [this, ans]() { answerClicked(ans); });
		return btn;
	};

	m_btnA = createBtn("A"); boxLayout->addWidget(m_btnA);
	m_btnB = createBtn("B"); boxLayout->addWidget(m_btnB);
	m_btnC = createBtn("C"); boxLayout->addWidget(m_btnC);
	m_btnD = createBtn("D"); boxLayout->addWidget(m_btnD);

	mainLayout->addWidget(centerBox);
}

void QuizWidget::setQuiz(const QString& q, const QString& a, const QString& b, const QString& c, const QString& d)
{
	m_questionLabel->setText(q);
	m_btnA->setText(QString("A) %1").arg(a)); m_btnA->setVisible(!a.isEmpty());
	m_btnB->setText(QString("B) %1").arg(b)); m_btnB->setVisible(!b.isEmpty());
	m_btnC->setText(QString("C) %1").arg(c)); m_btnC->setVisible(!c.isEmpty());
	m_btnD->setText(QString("D) %1").arg(d)); m_btnD->setVisible(!d.isEmpty());
}

void QuizWidget::answerClicked(const QString& ans)
{
	m_plugin->sendAnswerToMaster(ans);
	close();
}

void QuizWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	painter.fillRect(rect(), QColor(0, 0, 0, 180)); // Dim background
}
