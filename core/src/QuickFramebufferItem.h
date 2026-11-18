// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QQuickPaintedItem>
#include <memory>
#include "ComputerControlInterface.h"
class QuickVncView;
class VEYON_CORE_EXPORT QuickFramebufferItem : public QQuickPaintedItem
{
	Q_OBJECT
	Q_PROPERTY(QObject* computer READ computer WRITE setComputer NOTIFY computerChanged)
	Q_PROPERTY(QImage frame READ frame WRITE setFrame NOTIFY frameChanged)
	Q_PROPERTY(bool live READ live WRITE setLive NOTIFY liveChanged)
	Q_PROPERTY(bool viewOnly READ viewOnly WRITE setViewOnly NOTIFY viewOnlyChanged)
	Q_PROPERTY(QRect viewport READ viewport WRITE setViewport NOTIFY viewportChanged)
	Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)
public:
	explicit QuickFramebufferItem(QQuickItem* parent = nullptr);
	~QuickFramebufferItem() override;
	QObject* computer() const { return m_computer.data(); }
	void setComputer(QObject*);
	QImage frame() const { return m_frame; }
	void setFrame(const QImage&);
	bool live() const { return m_live; }
	void setLive(bool);
	bool viewOnly() const { return m_viewOnly; }
	void setViewOnly(bool);
	QRect viewport() const { return m_viewport; }
	void setViewport(QRect);
	QSize sourceSize() const;
	Q_INVOKABLE void sendShortcut(int);
	void paint(QPainter*) override;
Q_SIGNALS:
	void computerChanged();
	void frameChanged();
	void liveChanged();
	void viewOnlyChanged();
	void viewportChanged();
	void sourceSizeChanged();
protected:
	bool event(QEvent*) override;
	void geometryChange(const QRectF&, const QRectF&) override;
	void focusInEvent(QFocusEvent*) override;
	void focusOutEvent(QFocusEvent*) override;
private:
	friend class QuickVncView;
	void reconnect();
	void releaseView();
	void refresh();
	void updateInputState();
	QRectF contentRect() const;
	ComputerControlInterface::Pointer m_computer;
	std::unique_ptr<QuickVncView> m_view;
	QImage m_frame;
	QRect m_viewport;
	QPointF m_lastPointer;
	Qt::MouseButtons m_buttons{Qt::NoButton};
	bool m_live{false};
	bool m_viewOnly{true};
};
