/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "KWinKeyboard.h"

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon-names.h>
#include <array>
#include <limits>

KWinKeyboard::KWinKeyboard() = default;
KWinKeyboard::~KWinKeyboard() = default;
void KWinKeyboard::Deleter::operator()(xkb_keymap* map) const { xkb_keymap_unref(map); }
void KWinKeyboard::Deleter::operator()(xkb_state* state) const { xkb_state_unref(state); }
bool KWinKeyboard::isValid() const { return m_keymap != nullptr; }

bool KWinKeyboard::setKeymap(const QByteArray& data)
{
	m_keymap.reset();
	if (data.isEmpty() || data.size() > 1024 * 1024 || data.indexOf('\0') != data.size() - 1) { return false; }
	auto* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!context) { return false; }
	m_keymap.reset(xkb_keymap_new_from_string(context, data.constData(), XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS));
	xkb_context_unref(context);
	return isValid();
}

std::optional<KWinKeyboard::Key> KWinKeyboard::translate(quint32 symbol) const
{
	if (!m_keymap || symbol == XKB_KEY_NoSymbol) { return {}; }
	auto* map = m_keymap.get();
	const auto first = xkb_keymap_min_keycode(map);
	const auto last = xkb_keymap_max_keycode(map);
	if (last > 0xffff || last < first) { return {}; }
	const auto findModifier = [&](xkb_keysym_t symbol) -> quint32 {
		for (auto code = first; code <= last; ++code)
		{
			const auto layouts = xkb_keymap_num_layouts_for_key(map, code);
			for (xkb_layout_index_t layout = 0; layout < layouts; ++layout)
			{
				const xkb_keysym_t* symbols = nullptr;
				const auto count = xkb_keymap_key_get_syms_by_level(map, code, layout, 0, &symbols);
				for (int i = 0; i < count; ++i) { if (symbols[i] == symbol && code >= 8) { return code - 8; } }
			}
		}
		return uint32_t(-1);
	};
	const auto shiftKey = findModifier(XKB_KEY_Shift_L);
	const auto level3Key = findModifier(XKB_KEY_ISO_Level3_Shift);
	const auto modBit = [&](const char* name) -> xkb_mod_mask_t {
		const auto index = xkb_keymap_mod_get_index(map, name);
		return index < 32 ? xkb_mod_mask_t(1) << index : 0;
	};
	const auto shiftMask = modBit(XKB_MOD_NAME_SHIFT);
	const auto level3Mask = modBit(XKB_MOD_NAME_MOD5);
	const auto capsLockMask = m_capsLock ? modBit(XKB_MOD_NAME_CAPS) : 0;
	std::unique_ptr<xkb_state, Deleter> state(m_capsLock ? xkb_state_new(map) : nullptr);
	if (m_capsLock && !state) { return {}; }
	const auto numLockMask = modBit(XKB_MOD_NAME_NUM) | modBit("NumLock");
	const bool keypadSymbol = symbol >= XKB_KEY_KP_Space && symbol <= XKB_KEY_KP_Equal;
	std::optional<Key> result;
	int bestScore = std::numeric_limits<int>::max();
	const auto unicode = (symbol & 0xff000000) == 0x01000000 ? xkb_keysym_to_utf32(symbol) : 0;
	for (auto code = first; code <= last; ++code)
	{
		if (code < 8 || code - 8 > 0x2ff) { continue; }
		const auto layouts = xkb_keymap_num_layouts_for_key(map, code);
		for (xkb_layout_index_t layout = 0; layout < layouts; ++layout)
		{
			const auto levels = xkb_keymap_num_levels_for_key(map, code, layout);
			for (xkb_level_index_t level = 0; level < levels; ++level)
			{
				const xkb_keysym_t* symbols = nullptr;
				const auto count = xkb_keymap_key_get_syms_by_level(map, code, layout, level, &symbols);
				if (count != 1 || (symbols[0] != symbol && (!unicode || xkb_keysym_to_utf32(symbols[0]) != unicode))) { continue; }
				std::array<xkb_mod_mask_t, 16> masks{};
				const auto maskCount = xkb_keymap_key_get_mods_for_level(map, code, layout, level, masks.data(), masks.size());
				for (size_t i = 0; i < maskCount; ++i)
				{
					auto mask = masks[i];
					if (mask & ~(shiftMask | level3Mask | (keypadSymbol ? numLockMask : 0))) { continue; }
					if (capsLockMask && !keypadSymbol)
					{
						const auto producesSymbol = [&](xkb_mod_mask_t modifiers) {
							xkb_state_update_mask(state.get(), modifiers, 0, capsLockMask, 0, 0, layout);
							const auto actual = xkb_state_key_get_one_sym(state.get(), code);
							return actual == symbol || (unicode && xkb_keysym_to_utf32(actual) == unicode);
						};
						if (!producesSymbol(mask))
						{
							mask ^= shiftMask;
							if (!producesSymbol(mask)) { continue; }
						}
					}
					Key key{code - 8, {}};
					if (mask & shiftMask)
					{
						if (shiftKey == uint32_t(-1)) { continue; }
						key.modifiers.append(shiftKey);
					}
					if (mask & level3Mask)
					{
						if (level3Key == uint32_t(-1)) { continue; }
						key.modifiers.append(level3Key);
					}
					const auto score = int(key.modifiers.size()) * 1000 + int(layout) + (symbols[0] == symbol ? 0 : 100);
					if (score < bestScore) { result = key; bestScore = score; }
				}
			}
		}
	}
	return result;
}
