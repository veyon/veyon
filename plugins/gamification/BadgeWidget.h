#pragma once

#include <QWidget>
#include <QTimer>
#include <QPropertyAnimation>

class BadgeWidget : public QWidget
{
	Q_OBJECT
public:
	explicit BadgeWidget(QWidget* parent = nullptr);
	
	void showBadge(const QString& text, const QString& color);

protected:
	void paintEvent(QPaintEvent* event) override;

private Q_SLOTS:
	void startFadeOut();

private:
	QString m_text;
	QString m_color;
	QTimer* m_timer;
	QPropertyAnimation* m_animation;
};
