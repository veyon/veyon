#pragma once

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QColor>

class QPushButton;

class WhiteboardWidget : public QWidget
{
	Q_OBJECT
public:
	explicit WhiteboardWidget(QWidget* parent = nullptr);
	~WhiteboardWidget() override = default;

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;

private Q_SLOTS:
	void clearBoard();
	void closeBoard();

private:
	void drawLineTo(const QPoint& endPoint);

	bool m_drawing = false;
	QPoint m_lastPoint;
	QPixmap m_pixmap;
	QColor m_penColor = Qt::red;
	int m_penWidth = 5;
	
	QPushButton* m_clearButton;
	QPushButton* m_closeButton;
};
