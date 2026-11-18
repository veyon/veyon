// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QHash>
#include <QKeyEvent>
#include <QList>
#include <functional>

class QuickKeyEventTracker
{
public:
	using Sender = std::function<void(QKeyEvent*)>;

	void forward(QKeyEvent* event, const Sender& send)
	{
		const auto identity = event->nativeScanCode()
			? (quint64(event->nativeScanCode()) << 1) | 1
			: quint64(quint32(event->key())) << 1;
		auto held = m_pressed.find(identity);
		if (event->type() == QEvent::KeyPress && held == m_pressed.end()) {
			held = m_pressed.insert(identity, {event->key(), event->modifiers(), event->nativeScanCode(),
				event->nativeVirtualKey(), event->nativeModifiers(), event->text()});
			m_order.append(identity);
		}
		if (held == m_pressed.end()) {
			send(event);
			return;
		}
		const auto key = held.value();
		QKeyEvent normalized(event->type(), key.key, key.modifiers, key.scanCode,
			key.virtualKey, key.nativeModifiers, key.text, event->isAutoRepeat(), quint16(event->count()));
		if (event->type() == QEvent::KeyRelease && !event->isAutoRepeat()) {
			m_pressed.erase(held);
			m_order.removeAll(identity);
		}
		send(&normalized);
	}

	void releaseAll(const Sender& send)
	{
		const auto pressed = m_pressed;
		const auto order = m_order;
		m_pressed.clear();
		m_order.clear();
		for (auto it = order.crbegin(); it != order.crend(); ++it) {
			const auto key = pressed.value(*it);
			QKeyEvent release(QEvent::KeyRelease, key.key, key.modifiers, key.scanCode,
				key.virtualKey, key.nativeModifiers, key.text);
			send(&release);
		}
	}

private:
	struct PressedKey {
		int key;
		Qt::KeyboardModifiers modifiers;
		quint32 scanCode;
		quint32 virtualKey;
		quint32 nativeModifiers;
		QString text;
	};
	QHash<quint64, PressedKey> m_pressed;
	QList<quint64> m_order;
};
