// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QList>
#include <QString>

// Parses user-configurable key sequences which are sent to the Windows sign-in
// screen before entering the credentials, e.g. to switch from the tile of the
// last signed-in user to "Other user" on Windows 10/11.
//
// Syntax: steps separated by whitespace or commas. Each step is either
//   - a key name (Tab, Return/Enter, Escape/Esc, Space, Backspace, Delete/Del,
//     Up, Down, Left, Right, Home, End, PageUp, PageDown, Insert, F1-F12) or
//     a single character (a-z, 0-9),
//   - optionally prefixed by modifiers joined with "+" (Shift, Ctrl/Control,
//     Alt, Win), e.g. "Shift+Tab" or "Ctrl+A",
//   - or a pause in milliseconds: "Wait:500" (also "Sleep:500", "Delay:500").
// Names are case-insensitive.
//
// Key codes are X11 keysyms as expected by vncKeymap/DesktopInputController.
class LogonKeySequence
{
public:
	using KeyCode = uint32_t;

	struct Step
	{
		QList<KeyCode> modifiers;
		KeyCode key = 0;
		int delay = 0; // in milliseconds, step is a pause if > 0 (key is 0 then)

		bool operator==(const Step& other) const
		{
			return modifiers == other.modifiers && key == other.key && delay == other.delay;
		}
	};

	using Steps = QList<Step>;

	static constexpr KeyCode Key_BackSpace = 0xff08;
	static constexpr KeyCode Key_Tab = 0xff09;
	static constexpr KeyCode Key_Return = 0xff0d;
	static constexpr KeyCode Key_Escape = 0xff1b;
	static constexpr KeyCode Key_Home = 0xff50;
	static constexpr KeyCode Key_Left = 0xff51;
	static constexpr KeyCode Key_Up = 0xff52;
	static constexpr KeyCode Key_Right = 0xff53;
	static constexpr KeyCode Key_Down = 0xff54;
	static constexpr KeyCode Key_PageUp = 0xff55;
	static constexpr KeyCode Key_PageDown = 0xff56;
	static constexpr KeyCode Key_End = 0xff57;
	static constexpr KeyCode Key_Insert = 0xff63;
	static constexpr KeyCode Key_F1 = 0xffbe;
	static constexpr KeyCode Key_Shift = 0xffe1;
	static constexpr KeyCode Key_Control = 0xffe3;
	static constexpr KeyCode Key_Alt = 0xffe9;
	static constexpr KeyCode Key_Super = 0xffeb;
	static constexpr KeyCode Key_Delete = 0xffff;
	static constexpr KeyCode Key_Space = 0x20;

	static constexpr int MaximumDelay = 30000;

	// returns false and sets errorMessage (if non-null) for invalid sequences
	static bool parse(const QString& sequence, Steps* steps, QString* errorMessage = nullptr);

};
