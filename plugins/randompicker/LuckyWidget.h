#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>

class LuckyWidget : public QWidget
{
	Q_OBJECT
public:
	explicit LuckyWidget(QWidget* parent = nullptr);
	~LuckyWidget() override = default;

	void playAnimation();

protected:
	void paintEvent(QPaintEvent* event) override;

private Q_SLOTS:
	void hideAnimation();

private:
	QLabel* m_messageLabel;
	QTimer m_hideTimer;
};
