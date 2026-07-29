#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>

class QuickQuizPlugin;

class QuizWidget : public QWidget
{
	Q_OBJECT
public:
	explicit QuizWidget(QuickQuizPlugin* plugin, QWidget* parent = nullptr);
	~QuizWidget() override = default;

	void setQuiz(const QString& q, const QString& a, const QString& b, const QString& c, const QString& d);

protected:
	void paintEvent(QPaintEvent* event) override;

private Q_SLOTS:
	void answerClicked(const QString& ans);

private:
	QuickQuizPlugin* m_plugin;
	QLabel* m_questionLabel;
	QPushButton* m_btnA;
	QPushButton* m_btnB;
	QPushButton* m_btnC;
	QPushButton* m_btnD;
};
