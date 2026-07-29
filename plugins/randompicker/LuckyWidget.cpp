#include "LuckyWidget.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QApplication>
#include <QScreen>

LuckyWidget::LuckyWidget(QWidget* parent) : QWidget(parent)
{
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setAlignment(Qt::AlignCenter);

	m_messageLabel = new QLabel(tr("🎉 ŞANSLI KİŞİ SENSİN! 🎉"), this);
	m_messageLabel->setStyleSheet("font-size: 64px; font-weight: bold; color: white; "
								  "background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #ff007f, stop:1 #7f00ff);"
								  "border-radius: 30px; padding: 50px;");
	m_messageLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(m_messageLabel);

	m_hideTimer.setSingleShot(true);
	connect(&m_hideTimer, &QTimer::timeout, this, &LuckyWidget::hideAnimation);
}

void LuckyWidget::playAnimation()
{
	QRect screenGeometry = QApplication::primaryScreen()->geometry();
	resize(screenGeometry.size());
	move(screenGeometry.topLeft());
	showFullScreen();
	activateWindow();
	
	m_hideTimer.start(5000); // 5 saniye sonra kapanır
}

void LuckyWidget::hideAnimation()
{
	close();
}

void LuckyWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	painter.fillRect(rect(), QColor(0, 0, 0, 160)); // Yarı saydam siyah arka plan
}
