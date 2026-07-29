#include "WhiteboardWidget.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

WhiteboardWidget::WhiteboardWidget(QWidget* parent) : QWidget(parent)
{
	// Make it transparent but able to receive clicks
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
	setAttribute(Qt::WA_TranslucentBackground);
	setCursor(Qt::CrossCursor);

	// Setup UI buttons
	m_clearButton = new QPushButton(tr("Temizle"), this);
	m_clearButton->setStyleSheet("background-color: white; border: 2px solid red; border-radius: 5px; padding: 10px; font-weight: bold;");
	connect(m_clearButton, &QPushButton::clicked, this, &WhiteboardWidget::clearBoard);

	m_closeButton = new QPushButton(tr("Kapat (Esc)"), this);
	m_closeButton->setStyleSheet("background-color: white; border: 2px solid black; border-radius: 5px; padding: 10px; font-weight: bold;");
	connect(m_closeButton, &QPushButton::clicked, this, &WhiteboardWidget::closeBoard);

	QHBoxLayout* topLayout = new QHBoxLayout();
	topLayout->addStretch();
	topLayout->addWidget(m_clearButton);
	topLayout->addWidget(m_closeButton);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(topLayout);
	mainLayout->addStretch();
}

void WhiteboardWidget::clearBoard()
{
	m_pixmap.fill(Qt::transparent);
	update();
}

void WhiteboardWidget::closeBoard()
{
	close();
}

void WhiteboardWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		m_lastPoint = event->pos();
		m_drawing = true;
	}
}

void WhiteboardWidget::mouseMoveEvent(QMouseEvent* event)
{
	if ((event->buttons() & Qt::LeftButton) && m_drawing) {
		drawLineTo(event->pos());
	}
}

void WhiteboardWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && m_drawing) {
		drawLineTo(event->pos());
		m_drawing = false;
	}
}

void WhiteboardWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	// Fill slightly visible background so teacher knows it's active
	painter.fillRect(rect(), QColor(255, 255, 255, 10)); 
	QRect dirtyRect = event->rect();
	painter.drawPixmap(dirtyRect, m_pixmap, dirtyRect);
}

void WhiteboardWidget::resizeEvent(QResizeEvent* event)
{
	if (width() > m_pixmap.width() || height() > m_pixmap.height()) {
		int newWidth = qMax(width() + 128, m_pixmap.width());
		int newHeight = qMax(height() + 128, m_pixmap.height());
		QPixmap newPixmap(newWidth, newHeight);
		newPixmap.fill(Qt::transparent);
		QPainter painter(&newPixmap);
		painter.drawPixmap(0, 0, m_pixmap);
		m_pixmap = newPixmap;
	}
	QWidget::resizeEvent(event);
}

void WhiteboardWidget::drawLineTo(const QPoint& endPoint)
{
	QPainter painter(&m_pixmap);
	painter.setPen(QPen(m_penColor, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	painter.drawLine(m_lastPoint, endPoint);
	
	int rad = (m_penWidth / 2) + 2;
	update(QRect(m_lastPoint, endPoint).normalized().adjusted(-rad, -rad, +rad, +rad));
	m_lastPoint = endPoint;
}

void WhiteboardWidget::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape) {
		closeBoard();
	}
	QWidget::keyPressEvent(event);
}
