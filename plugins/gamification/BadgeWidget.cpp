#include "BadgeWidget.h"
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QGraphicsOpacityEffect>
#include <QPainterPath>
#include <QPen>

BadgeWidget::BadgeWidget(QWidget* parent) : QWidget(parent)
{
	setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool | Qt::WindowTransparentForInput);
	setAttribute(Qt::WA_TranslucentBackground);
	
	m_timer = new QTimer(this);
	m_timer->setSingleShot(true);
	connect(m_timer, &QTimer::timeout, this, &BadgeWidget::startFadeOut);
	
	m_animation = new QPropertyAnimation(this, "windowOpacity");
	m_animation->setDuration(1000);
	m_animation->setStartValue(1.0);
	m_animation->setEndValue(0.0);
	connect(m_animation, &QPropertyAnimation::finished, this, &QWidget::hide);
}

void BadgeWidget::showBadge(const QString& text, const QString& color)
{
	m_text = text;
	m_color = color;
	
	// Set size
	setFixedSize(600, 250);
	
	// Center on screen
	QRect screenGeometry = QApplication::primaryScreen()->geometry();
	int x = (screenGeometry.width() - width()) / 2;
	int y = (screenGeometry.height() - height()) / 2;
	move(x, y);
	
	setWindowOpacity(1.0);
	show();
	
	// Display for 4 seconds before fading out
	m_timer->start(4000);
}

void BadgeWidget::startFadeOut()
{
	m_animation->start();
}

void BadgeWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	
	// Draw background
	QPainterPath path;
	path.addRoundedRect(rect(), 30, 30);
	painter.fillPath(path, QColor(0, 0, 0, 180));
	
	// Draw border
	painter.setPen(QPen(QColor(m_color), 6));
	painter.drawPath(path);
	
	// Draw text
	painter.setPen(QColor(255, 255, 255));
	QFont font = painter.font();
	font.setPointSize(42);
	font.setBold(true);
	painter.setFont(font);
	
	painter.drawText(rect(), Qt::AlignCenter, m_text);
}
