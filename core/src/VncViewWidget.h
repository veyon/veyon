// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QTimer>
#include <QWidget>

#include "ComputerControlInterface.h"
#include "VncView.h"

class VEYON_CORE_EXPORT VncViewWidget : public QWidget, public VncView
{
	Q_OBJECT
public:
	VncViewWidget(const ComputerControlInterface::Pointer& computerControlInterface, QRect viewport,
				  QWidget* parent);

	~VncViewWidget() override;

	QSize sizeHint() const override;

	void setViewOnly(bool enabled) override;

Q_SIGNALS:
	void mouseAtBorder();
	void sizeHintChanged();

protected:
	void updateView(int x, int y, int w, int h) override;
	QSize viewSize() const override;
	void setViewCursor(const QCursor& cursor) override;

	void updateGeometry() override;

	bool event(QEvent* handleEvent) override;
	bool eventFilter(QObject* obj, QEvent* handleEvent) override;
	void focusInEvent(QFocusEvent* handleEvent) override;
	void focusOutEvent(QFocusEvent* handleEvent) override;
	void mouseEventHandler(QMouseEvent* handleEvent) override;
	void paintEvent(QPaintEvent* handleEvent) override;
	void resizeEvent(QResizeEvent* handleEvent) override;

private:
	void drawBusyIndicator(QPainter* painter);
	void updateConnectionState();

	bool m_viewOnlyFocus{true};

	static constexpr auto BusyIndicatorUpdateInterval = 25;
	QTimer m_busyIndicatorTimer{this};
	int m_busyIndicatorState{0};

	static constexpr int MouseBorderSignalDelay = 500;
	QTimer m_mouseBorderSignalTimer{this};

	// the connection's image as a scaled view shows it, and the viewport it was made from
	QImage m_scaledImage{};
	QRect m_scaledImageSource{};
	bool m_scaledImageOutdated{true};
	bool m_forceOpaqueFramebuffer{false};
};
