/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include "KWinKeyboard.h"
#include <QHash>
#include <QRect>
#include <QSize>
#include <functional>

struct wl_registry;
struct wl_seat;
struct wl_keyboard;
struct org_kde_kwin_fake_input;
struct org_kde_kwin_keystate;

class KWinInput
{
public:
	KWinInput(std::function<void()> flush, std::function<void()> availabilityChanged);
	~KWinInput();
	void announce(wl_registry* registry, quint32 id, const char* name, quint32 version);
	void removeGlobal(quint32 id);
	void registryReady();
	void keyEvent(quintptr client, quint32 symbol, bool down);
	void pointerEvent(quintptr client, int buttons, int x, int y, const QRect& workspace, const QSize& frameSize);
	void releaseClient(quintptr client);
	void stop();
	bool pointerAvailable() const { return m_input != nullptr; }
	bool keyboardAvailable() const { return m_input && (m_version >= 6 || (m_keymap.isValid() && m_capsLockKnown)); }

private:
	struct Client { QHash<quint32, KWinKeyboard::Key> keys; int buttons{0}; };
	void setKeymap(int fd, quint32 format, quint32 size);
	void sendKey(quint32 code, bool down);
	void releaseKey(const KWinKeyboard::Key& key);
	void releaseAll();
	Client* clientState(quintptr client);
	org_kde_kwin_fake_input* m_input{nullptr};
	org_kde_kwin_keystate* m_lockState{nullptr};
	quint32 m_lockStateGlobal{0};
	bool m_capsLockKnown{false};
	wl_seat* m_seat{nullptr};
	wl_keyboard* m_keyboard{nullptr};
	quint32 m_inputGlobal{0};
	quint32 m_seatGlobal{0};
	quint32 m_version{0};
	KWinKeyboard m_keymap;
	QHash<quintptr, Client> m_clients;
	QHash<quint32, int> m_keys;
	QHash<quint32, int> m_buttons;
	std::function<void()> m_flush;
	std::function<void()> m_availabilityChanged;
};
