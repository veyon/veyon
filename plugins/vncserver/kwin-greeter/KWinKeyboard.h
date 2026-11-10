/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QByteArray>
#include <QVector>
#include <optional>
#include <memory>

struct xkb_keymap;
struct xkb_state;

class KWinKeyboard
{
public:
	struct Key { quint32 code; QVector<quint32> modifiers; };
	KWinKeyboard();
	~KWinKeyboard();
	bool setKeymap(const QByteArray& data);
	bool isValid() const;
	void setCapsLock(bool enabled) { m_capsLock = enabled; }
	std::optional<Key> translate(quint32 symbol) const;
private:
	struct Deleter
	{
		void operator()(xkb_keymap* map) const;
		void operator()(xkb_state* state) const;
	};
	std::unique_ptr<xkb_keymap, Deleter> m_keymap;
	bool m_capsLock{false};
};
