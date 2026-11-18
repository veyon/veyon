// SPDX-License-Identifier: GPL-2.0-or-later
#include "QuickFramebufferItem.h"
#include "QuickKeyEventTracker.h"
#include "VncView.h"
#include <QQuickWindow>
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QHoverEvent>
#include <QWheelEvent>
#include <QHash>
namespace {
struct LiveLease { int users{0}; ComputerControlInterface::UpdateMode previous; };
QHash<ComputerControlInterface*, LiveLease> liveLeases;
}
class QuickVncView : public VncView
{
public:
	QuickVncView(QuickFramebufferItem* owner, const ComputerControlInterface::Pointer& computer) : VncView(computer), m_owner(owner) { connectUpdateFunctions(owner); }
	bool forward(QEvent* event) {
		if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
			bool handled = false;
			m_keys.forward(static_cast<QKeyEvent*>(event), [this, &handled](QKeyEvent* key) { handled = handleEvent(key); });
			if (handled) event->accept();
			return handled;
		}
		if (event->type() == QEvent::FocusOut) releaseInput();
		return handleEvent(event);
	}
	void releaseInput() {
		m_keys.releaseAll([this](QKeyEvent* key) { keyEventHandler(key); });
		unpressAllModifiers();
		if (!viewOnly()) for (auto button : {Qt::LeftButton,Qt::MiddleButton,Qt::RightButton}) {
			if (!m_owner->m_buttons.testFlag(button)) continue;
			QMouseEvent event(QEvent::MouseButtonRelease,m_owner->m_lastPointer,m_owner->m_lastPointer,button,Qt::NoButton,Qt::NoModifier);
			mouseEventHandler(&event);
		}
		m_owner->m_buttons = Qt::NoButton;
	}
protected:
	void updateView(int, int, int, int) override { m_owner->refresh(); }
	QSize viewSize() const override { return m_owner->contentRect().size().toSize(); }
	void setViewCursor(const QCursor& cursor) override { m_owner->setCursor(cursor); }
	void updateGeometry() override { m_owner->refresh(); Q_EMIT m_owner->sourceSizeChanged(); }
private:
	QuickFramebufferItem* m_owner;
	QuickKeyEventTracker m_keys;
};
QuickFramebufferItem::QuickFramebufferItem(QQuickItem* parent) : QQuickPaintedItem(parent)
{
	setAcceptedMouseButtons(Qt::AllButtons);
	setAcceptHoverEvents(true);
	setActiveFocusOnTab(true);
	connect(this, &QQuickItem::visibleChanged, this, &QuickFramebufferItem::updateInputState);
	connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow* window) {
		if (window) connect(window, &QWindow::activeChanged, this, &QuickFramebufferItem::updateInputState, Qt::UniqueConnection);
		updateInputState();
	});
}
QuickFramebufferItem::~QuickFramebufferItem() { releaseView(); }
void QuickFramebufferItem::releaseView()
{
	if (!m_view) return;
	m_view->releaseInput();
	if (window()) window()->setKeyboardGrabEnabled(false);
	m_view.reset();
	auto it = liveLeases.find(m_computer.data());
	if (it != liveLeases.end()) {
		if (--it->users > 0) m_computer->setUpdateMode(ComputerControlInterface::UpdateMode::Live);
		else { const auto previous=it->previous; liveLeases.erase(it); m_computer->setUpdateMode(previous); }
	}
}
void QuickFramebufferItem::setComputer(QObject* object)
{
	auto computer = qobject_cast<ComputerControlInterface*>(object);
	if (computer == m_computer.data()) return;
	if (m_computer) {
		disconnect(m_computer.data(), nullptr, this, nullptr);
		if (m_computer->vncConnection()) disconnect(m_computer->vncConnection(), nullptr, this, nullptr);
	}
	releaseView();
	m_computer = computer ? computer->sharedFromThis() : ComputerControlInterface::Pointer{};
	if (computer && !m_computer) m_computer = computer->weakPointer();
	reconnect();
	Q_EMIT computerChanged();
}
void QuickFramebufferItem::setLive(bool live)
{
	if (live == m_live) return;
	m_live = live;
	if (m_computer && m_computer->vncConnection()) disconnect(m_computer->vncConnection(), nullptr, this, nullptr);
	releaseView();
	reconnect();
	Q_EMIT liveChanged();
}
void QuickFramebufferItem::reconnect()
{
	if (m_computer) {
		connect(m_computer.data(), &ComputerControlInterface::framebufferUpdated, this, &QuickFramebufferItem::refresh, Qt::UniqueConnection);
		connect(m_computer.data(), &ComputerControlInterface::framebufferSizeChanged, this, &QuickFramebufferItem::refresh, Qt::UniqueConnection);
		if (m_live) {
			auto& lease = liveLeases[m_computer.data()];
			if (lease.users++ == 0) lease.previous = m_computer->updateMode();
			m_view = std::make_unique<QuickVncView>(this, m_computer);
			m_view->setViewport(m_viewport);
			updateInputState();
		}
	}
	refresh();
}
void QuickFramebufferItem::setFrame(const QImage& frame)
{
	m_frame = frame;
	update();
	Q_EMIT frameChanged();
	Q_EMIT sourceSizeChanged();
}
void QuickFramebufferItem::refresh()
{
	if (m_computer) setFrame(m_live ? m_computer->framebuffer() : m_computer->scaledFramebuffer());
	else setFrame({});
}
void QuickFramebufferItem::setViewOnly(bool value)
{
	if (m_viewOnly == value) return;
	m_viewOnly = value;
	updateInputState();
	Q_EMIT viewOnlyChanged();
}
void QuickFramebufferItem::updateInputState()
{
	if (!m_view) return;
	const bool blocked = m_viewOnly || !isVisible() || !hasActiveFocus() || !window() || !window()->isActive();
	if (blocked) m_view->releaseInput();
	m_view->setViewOnly(blocked);
	if (window()) window()->setKeyboardGrabEnabled(!blocked);
}
void QuickFramebufferItem::setViewport(QRect viewport)
{
	if (viewport == m_viewport) return;
	m_viewport = viewport;
	if (m_view) m_view->setViewport(viewport);
	update();
	Q_EMIT viewportChanged();
	Q_EMIT sourceSizeChanged();
}
QSize QuickFramebufferItem::sourceSize() const { return m_viewport.isEmpty() ? m_frame.size() : m_viewport.size(); }
QRectF QuickFramebufferItem::contentRect() const
{
	const auto source = sourceSize();
	if (source.isEmpty() || width() <= 0 || height() <= 0) return {};
	auto size = QSizeF(source).scaled(QSizeF(width(), height()), Qt::KeepAspectRatio);
	if (m_live && size.width() > source.width()) size = source;
	return {(width()-size.width())/2, (height()-size.height())/2, size.width(), size.height()};
}
void QuickFramebufferItem::paint(QPainter* painter)
{
	painter->fillRect(boundingRect(), Qt::black);
	if (m_frame.isNull()) return;
	const auto source = m_viewport.isEmpty() ? m_frame.rect() : m_viewport;
	const auto target = contentRect();
	auto image = source == m_frame.rect() ? m_frame : m_frame.copy(source);
	if (target.width() < image.width() || target.height() < image.height()) image = image.scaled(target.size().toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	painter->setRenderHint(QPainter::SmoothPixmapTransform);
	painter->drawImage(target, image);
}
void QuickFramebufferItem::geometryChange(const QRectF& current, const QRectF& previous)
{
	QQuickPaintedItem::geometryChange(current, previous);
	update();
}
void QuickFramebufferItem::focusInEvent(QFocusEvent* event) { QQuickPaintedItem::focusInEvent(event); updateInputState(); }
void QuickFramebufferItem::focusOutEvent(QFocusEvent* event) { if (m_view) m_view->forward(event); QQuickPaintedItem::focusOutEvent(event); updateInputState(); }
void QuickFramebufferItem::sendShortcut(int shortcut)
{
	if (m_view && !m_viewOnly && shortcut >= 0 && shortcut < VncView::ShortcutCount) {
		forceActiveFocus(); updateInputState(); m_view->sendShortcut(static_cast<VncView::Shortcut>(shortcut));
	}
}
bool QuickFramebufferItem::event(QEvent* event)
{
	if (!m_view || m_viewOnly) return QQuickPaintedItem::event(event);
	const auto offset = contentRect().topLeft();
	if (contentRect().isEmpty()) return QQuickPaintedItem::event(event);
	switch (event->type()) {
	case QEvent::MouseButtonPress: forceActiveFocus(); updateInputState(); [[fallthrough]];
	case QEvent::MouseButtonRelease:
	case QEvent::MouseButtonDblClick:
	case QEvent::MouseMove: {
		auto e = static_cast<QMouseEvent*>(event);
		if ((e->type()==QEvent::MouseButtonPress || e->type()==QEvent::MouseButtonDblClick) && !contentRect().contains(e->position())) { event->accept(); return true; }
		const auto point = e->position()-offset;
		m_lastPointer = QPointF(qBound(0.0,point.x(),contentRect().width()-1),qBound(0.0,point.y(),contentRect().height()-1));
		m_buttons = e->buttons();
		QMouseEvent mapped(e->type(), m_lastPointer, e->globalPosition(), e->button(), e->buttons(), e->modifiers());
		if (!m_view->viewOnly()) m_view->forward(&mapped);
		event->accept(); return true;
	}
	case QEvent::HoverMove: {
		auto e = static_cast<QHoverEvent*>(event);
		QMouseEvent mapped(QEvent::MouseMove, e->position()-offset, e->globalPosition(), Qt::NoButton, Qt::NoButton, e->modifiers());
		if (!m_view->viewOnly()) m_view->forward(&mapped);
		return true;
	}
	case QEvent::Wheel: {
		auto e = static_cast<QWheelEvent*>(event);
		QWheelEvent mapped(e->position()-offset, e->globalPosition(), e->pixelDelta(), e->angleDelta(), e->buttons(), e->modifiers(), e->phase(), e->inverted());
		if (!m_view->viewOnly()) m_view->forward(&mapped);
		event->accept(); return true;
	}
	case QEvent::ShortcutOverride: event->accept(); return true;
	case QEvent::KeyPress:
	case QEvent::KeyRelease:
		if (!m_view->viewOnly()) return m_view->forward(event);
		break;
	default: break;
	}
	return QQuickPaintedItem::event(event);
}
