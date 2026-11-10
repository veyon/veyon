/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "KWinInput.h"
#include "wayland-fake-input-client.h"
#include "wayland-keystate-client.h"

#include <QDebug>
#include <QFile>
#include <QScopeGuard>
#include <algorithm>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/input-event-codes.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

namespace
{
// RFB bits 3..6 are wheel directions; the remaining bits are buttons.
constexpr quint32 ButtonCodes[] = {BTN_LEFT, BTN_MIDDLE, BTN_RIGHT, 0, 0, 0, 0, BTN_SIDE, BTN_EXTRA};

quint32 normalizedSymbol(quint32 symbol)
{
	const auto lower = xkb_keysym_to_lower(symbol);
	const auto unicode = xkb_keysym_to_utf32(lower);
	return unicode >= 0x20 ? 0x01000000 | unicode : lower;
}

void releaseKeyboard(wl_keyboard*& keyboard)
{
	if (!keyboard) { return; }
	if (wl_proxy_get_version(reinterpret_cast<wl_proxy*>(keyboard)) >= WL_KEYBOARD_RELEASE_SINCE_VERSION)
	{
		wl_keyboard_release(keyboard);
	}
	else { wl_keyboard_destroy(keyboard); }
	keyboard = nullptr;
}
}

KWinInput::KWinInput(std::function<void()> flush, std::function<void()> availabilityChanged) :
	m_flush(std::move(flush)), m_availabilityChanged(std::move(availabilityChanged)) {}
KWinInput::~KWinInput() { stop(); }

void KWinInput::announce(wl_registry* registry, quint32 id, const char* name, quint32 version)
{
	if (std::strcmp(name, org_kde_kwin_fake_input_interface.name) == 0 && version >= 5 && !m_input)
	{
		m_version = std::min(version, quint32(6));
		m_input = static_cast<org_kde_kwin_fake_input*>(wl_registry_bind(registry, id, &org_kde_kwin_fake_input_interface, m_version));
		m_inputGlobal = id;
		org_kde_kwin_fake_input_authenticate(m_input, "Veyon", "Remote control of the login screen through authenticated Veyon connections");
		m_availabilityChanged();
	}
	else if (std::strcmp(name, org_kde_kwin_keystate_interface.name) == 0 && version >= 1 && !m_lockState)
	{
		m_lockState = static_cast<org_kde_kwin_keystate*>(wl_registry_bind(registry, id, &org_kde_kwin_keystate_interface, 1));
		m_lockStateGlobal = id;
		static const org_kde_kwin_keystate_listener listener = {
			.stateChanged = [](void* data, org_kde_kwin_keystate*, uint32_t key, uint32_t state) {
				if (key != ORG_KDE_KWIN_KEYSTATE_KEY_CAPSLOCK || state > ORG_KDE_KWIN_KEYSTATE_STATE_LOCKED) { return; }
				auto* self = static_cast<KWinInput*>(data);
				self->m_keymap.setCapsLock(state != ORG_KDE_KWIN_KEYSTATE_STATE_UNLOCKED);
				self->m_capsLockKnown = true;
				self->m_availabilityChanged();
			},
		};
		org_kde_kwin_keystate_add_listener(m_lockState, &listener, this);
		org_kde_kwin_keystate_fetchStates(m_lockState);
	}
	else if (std::strcmp(name, wl_seat_interface.name) == 0 && !m_seat)
	{
		m_seat = static_cast<wl_seat*>(wl_registry_bind(registry, id, &wl_seat_interface, std::min(version, quint32(5))));
		m_seatGlobal = id;
		static const wl_seat_listener listener = {
			.capabilities = [](void* data, wl_seat* seat, uint32_t capabilities) {
				auto* self = static_cast<KWinInput*>(data);
				if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !self->m_keyboard)
				{
					self->m_keyboard = wl_seat_get_keyboard(seat);
					static const wl_keyboard_listener keyboardListener = {
						.keymap = [](void* data, wl_keyboard*, uint32_t format, int32_t fd, uint32_t size) {
							static_cast<KWinInput*>(data)->setKeymap(fd, format, size);
						},
						.enter = [](void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*) {},
						.leave = [](void*, wl_keyboard*, uint32_t, wl_surface*) {},
						.key = [](void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t) {},
						.modifiers = [](void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {},
						.repeat_info = [](void*, wl_keyboard*, int32_t, int32_t) {},
					};
					wl_keyboard_add_listener(self->m_keyboard, &keyboardListener, self);
				}
				else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && self->m_keyboard)
				{
					self->releaseAll();
					releaseKeyboard(self->m_keyboard);
					self->m_keymap.setKeymap({});
					self->m_availabilityChanged();
				}
			},
			.name = [](void*, wl_seat*, const char*) {},
		};
		wl_seat_add_listener(m_seat, &listener, this);
	}
}

void KWinInput::registryReady()
{
	if (!m_input) { qWarning() << "KWin fake-input is unavailable; greeter capture remains view-only"; }
	else if (m_version < 6 && !m_lockState)
	{
		qWarning() << "KWin greeter CapsLock state unavailable; legacy keyboard control is disabled";
	}
}

void KWinInput::setKeymap(int fd, quint32 format, quint32 size)
{
	const auto closeFd = qScopeGuard([fd]() { if (fd >= 0) { ::close(fd); } });
	releaseAll();
	m_keymap.setKeymap({});
	struct stat info{};
	QFile file;
	if (fd >= 0 && format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 && size > 0 && size <= 1024 * 1024 &&
		::fstat(fd, &info) == 0 && S_ISREG(info.st_mode) && info.st_size >= qint64(size) &&
		file.open(fd, QIODevice::ReadOnly, QFileDevice::DontCloseHandle) && file.seek(0))
	{
		const auto bytes = file.read(size);
		if (bytes.size() == qint64(size)) { m_keymap.setKeymap(bytes); }
	}
	if (!m_keymap.isValid() && m_version < 6)
	{
		qWarning() << "KWin greeter keyboard keymap unavailable; keyboard control is disabled";
	}
	m_availabilityChanged();
}

void KWinInput::removeGlobal(quint32 id)
{
	if (id == m_lockStateGlobal && m_lockState)
	{
		if (m_version < 6) { releaseAll(); }
		wl_proxy_destroy(reinterpret_cast<wl_proxy*>(m_lockState));
		m_lockState = nullptr;
		m_lockStateGlobal = 0;
		m_capsLockKnown = false;
		m_keymap.setCapsLock(false);
		m_availabilityChanged();
	}
	else if (id == m_inputGlobal || id == m_seatGlobal) { stop(); m_availabilityChanged(); }
}

KWinInput::Client* KWinInput::clientState(quintptr client)
{
	if (!m_clients.contains(client) && m_clients.size() >= 64) { return nullptr; }
	return &m_clients[client];
}

void KWinInput::sendKey(quint32 code, bool down)
{
	if (m_version >= 6) { org_kde_kwin_fake_input_keyboard_keysym(m_input, code, down ? 1 : 0); }
	else { org_kde_kwin_fake_input_keyboard_key(m_input, code, down ? 1 : 0); }
}

void KWinInput::keyEvent(quintptr client, quint32 symbol, bool down)
{
	if (!keyboardAvailable()) { return; }
	auto* state = clientState(client);
	if (!state) { return; }
	if (!down)
	{
		auto key = state->keys.find(symbol);
		if (key == state->keys.end())
		{
			// Releasing Shift before a letter can change its release keysym.
			// Match only keys held by this client, never another client's state.
			const auto translated = m_version < 6 ? m_keymap.translate(symbol) : std::optional<KWinKeyboard::Key>{};
			for (auto it = state->keys.begin(); it != state->keys.end(); ++it)
			{
				if (m_version >= 6 ? normalizedSymbol(it.key()) == normalizedSymbol(symbol) :
					(translated && it.value().code == translated->code)) { key = it; break; }
			}
		}
		if (key == state->keys.end()) { return; }
		releaseKey(key.value());
		state->keys.erase(key);
		m_flush();
		return;
	}
	if (state->keys.contains(symbol) || state->keys.size() >= 256 || symbol == 0) { return; }
	std::optional<KWinKeyboard::Key> key;
	if (m_version >= 6) { key = KWinKeyboard::Key{symbol, {}}; }
	else { key = m_keymap.translate(symbol); }
	if (!key) { return; } // Never log typed characters or credentials.
	for (auto it = state->keys.constBegin(); it != state->keys.constEnd(); ++it)
	{
		if (m_version >= 6 ? normalizedSymbol(it.key()) == normalizedSymbol(symbol) : it.value().code == key->code) { return; }
	}
	state->keys.insert(symbol, *key);
	for (const auto modifier : key->modifiers) { if (m_keys[modifier]++ == 0) { sendKey(modifier, true); } }
	if (m_keys[key->code]++ == 0) { sendKey(key->code, true); }
	m_flush();
}

void KWinInput::releaseKey(const KWinKeyboard::Key& key)
{
	const auto release = [this](quint32 code) {
		auto it = m_keys.find(code);
		if (it != m_keys.end() && --it.value() == 0) { sendKey(code, false); m_keys.erase(it); }
	};
	release(key.code);
	for (auto it = key.modifiers.crbegin(); it != key.modifiers.crend(); ++it) { release(*it); }
}

void KWinInput::pointerEvent(quintptr client, int buttons, int x, int y, const QRect& workspace, const QSize& frameSize)
{
	if (!m_input || !workspace.isValid() || frameSize.isEmpty()) { return; }
	auto* state = clientState(client);
	if (!state) { return; }
	const double absoluteX = workspace.x() + double(std::clamp(x, 0, frameSize.width() - 1)) * workspace.width() / frameSize.width();
	const double absoluteY = workspace.y() + double(std::clamp(y, 0, frameSize.height() - 1)) * workspace.height() / frameSize.height();
	org_kde_kwin_fake_input_pointer_motion_absolute(m_input, wl_fixed_from_double(absoluteX), wl_fixed_from_double(absoluteY));
	buttons &= 0x1ff;
	for (int bit = 0; bit < 9; ++bit)
	{
		const bool pressed = (buttons & (1 << bit)) != 0;
		if (pressed == ((state->buttons & (1 << bit)) != 0)) { continue; }
		if (ButtonCodes[bit])
		{
			const auto code = ButtonCodes[bit];
			if (pressed)
			{
				if (m_buttons[code]++ == 0) { org_kde_kwin_fake_input_button(m_input, code, WL_POINTER_BUTTON_STATE_PRESSED); }
			}
			else
			{
				auto it = m_buttons.find(code);
				if (it != m_buttons.end() && --it.value() == 0)
				{
					org_kde_kwin_fake_input_button(m_input, code, WL_POINTER_BUTTON_STATE_RELEASED);
					m_buttons.erase(it);
				}
			}
		}
		else if (pressed)
		{
			const auto axis = bit < 5 ? WL_POINTER_AXIS_VERTICAL_SCROLL : WL_POINTER_AXIS_HORIZONTAL_SCROLL;
			const auto delta = bit == 3 || bit == 5 ? -15 : 15;
			org_kde_kwin_fake_input_axis(m_input, axis, wl_fixed_from_int(delta));
		}
	}
	state->buttons = buttons;
	m_flush();
}

void KWinInput::releaseClient(quintptr client)
{
	const auto found = m_clients.find(client);
	if (found == m_clients.end()) { return; }
	const auto state = found.value();
	m_clients.erase(found);
	for (const auto& key : state.keys) { releaseKey(key); }
	for (int bit = 0; bit < 9; ++bit)
	{
		if (!ButtonCodes[bit] || !(state.buttons & (1 << bit))) { continue; }
		auto it = m_buttons.find(ButtonCodes[bit]);
		if (it != m_buttons.end() && --it.value() == 0)
		{
			org_kde_kwin_fake_input_button(m_input, it.key(), WL_POINTER_BUTTON_STATE_RELEASED);
			m_buttons.erase(it);
		}
	}
	m_flush();
}

void KWinInput::releaseAll()
{
	const auto clients = m_clients.keys();
	for (auto client : clients) { releaseClient(client); }
}

void KWinInput::stop()
{
	releaseAll();
	releaseKeyboard(m_keyboard);
	if (m_lockState) { wl_proxy_destroy(reinterpret_cast<wl_proxy*>(m_lockState)); m_lockState = nullptr; }
	m_lockStateGlobal = 0;
	m_capsLockKnown = false;
	m_keymap.setCapsLock(false);
	if (m_seat)
	{
		if (wl_proxy_get_version(reinterpret_cast<wl_proxy*>(m_seat)) >= WL_SEAT_RELEASE_SINCE_VERSION) { wl_seat_release(m_seat); }
		else { wl_seat_destroy(m_seat); }
		m_seat = nullptr;
	}
	if (m_input) { org_kde_kwin_fake_input_destroy(m_input); m_input = nullptr; }
	m_inputGlobal = m_seatGlobal = 0;
	m_keymap.setKeymap({});
	m_version = 0;
	m_keys.clear();
	m_buttons.clear();
}
