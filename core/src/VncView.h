// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QEvent>
#include <QPixmap>

#include "ComputerControlInterface.h"
#include "KeyboardShortcutTrapper.h"

class QHoverEvent;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class KeyboardShortcutTrapper;

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT VncView
{
public:
	using KeyCode = VncConnection::KeyCode;

	enum Shortcut {
		ShortcutCtrlAltDel,
		ShortcutCtrlEscape,
		ShortcutAltTab,
		ShortcutAltF4,
		ShortcutWinTab,
		ShortcutWin,
		ShortcutMenu,
		ShortcutAltCtrlF1,
		ShortcutCount
	};

	VncView(const ComputerControlInterface::Pointer& computerControlInterface);
	virtual ~VncView();

	ComputerControlInterface::Pointer computerControlInterface() const
	{
		return m_computerControlInterface;
	}

	VncConnection* connection() const
	{
		return m_connection;
	}

	bool viewOnly() const
	{
		return m_viewOnly;
	}

	QSize scaledSize() const;

	QSize effectiveFramebufferSize() const;

	const QRect& viewport() const
	{
		return m_viewport;
	}

	void setViewport(QRect viewport);

	virtual void setViewOnly(bool viewOnly);
	void sendShortcut(VncView::Shortcut shortcut);

protected:
	template<class SubClass>
	void connectUpdateFunctions(SubClass* object)
	{
		QObject::connect(connection(), &VncConnection::imageUpdated, object,
						 [this](int x, int y, int w, int h) { updateImage(x, y, w, h); });
		QObject::connect(connection(), &VncConnection::framebufferSizeChanged, object,
						 [this](int w, int h) { updateFramebufferSize(w, h); });

		QObject::connect(connection(), &VncConnection::cursorShapeUpdated, object,
						 [this](const QImage& cursorShape, int xh, int yh) { updateCursorShape(cursorShape, xh, yh); });
	}

	virtual void updateView(int x, int y, int w, int h) = 0;
	virtual QSize viewSize() const = 0;
	virtual void setViewCursor(const QCursor& cursor) = 0;
	virtual void updateGeometry() = 0;

	void updateCursorShape(const QImage& cursorShape, int xh, int yh);
	void updateFramebufferSize(int w, int h);
	void updateImage(int x, int y, int w, int h);

	void pressModifiers(const QList<KeyCode>& modifierKeyCodes);
	void unpressModifiers(const QList<KeyCode>& modifierKeyCodes);
	void unpressAllModifiers();

	void handleShortcut(KeyboardShortcutTrapper::Shortcut shortcut);
	bool handleEvent(QEvent* handleEvent);

	virtual void hoverEventHandler(QHoverEvent* event);
	virtual void keyEventHandler(QKeyEvent* event);
	virtual void mouseEventHandler(QMouseEvent* event);
	virtual void wheelEventHandler(QWheelEvent* event);

	bool isScaledView() const;

	QImage cursorShape() const
	{
		return m_cursorShape;
	}

	qreal scaleFactor() const;
	QPoint mapToFramebuffer(QPoint pos);
	QRect mapFromFramebuffer(QRect rect);

	void updateLocalCursor();

private:
	void pressKey(KeyCode key);
	void unpressKey(KeyCode key);

	ComputerControlInterface::Pointer m_computerControlInterface;
	ComputerControlInterface::UpdateMode m_previousUpdateMode;

	VncConnection* m_connection{nullptr};
	QImage m_cursorShape{};
	QPoint m_cursorHot{0, 0};
	QSize m_framebufferSize{0, 0};
	bool m_viewOnly{true};

	QRect m_viewport{};

	uint m_buttonMask{0};
	QMap<KeyCode, bool> m_modifierKeys;

	KeyboardShortcutTrapper* m_keyboardShortcutTrapper{nullptr};
};
