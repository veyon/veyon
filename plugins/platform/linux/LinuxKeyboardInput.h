// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

// clazy:excludeall=copyable-polymorphic

class LinuxKeyboardInput
{
public:
	using FakeKey = struct FakeKey;
	using Display = struct _XDisplay;

	LinuxKeyboardInput();
	~LinuxKeyboardInput();

	void pressAndReleaseKey(uint32_t keysym);

	void pressAndReleaseKey(const QByteArray& utf8Data);

	void sendString(const QString& string, int keyPressInterval);

private:
	// X11/libfakekey path
	Display* m_display{nullptr};
	FakeKey* m_fakeKeyHandle{nullptr};

	// Wayland/uinput path
	int m_uinputFd{-1};
	bool m_uinputAvailable{false};
	const bool m_isWaylandSession;

	void initUinput();
	void cleanupUinput();
	void pressAndReleaseKeyUinput(uint32_t linuxKeycode);
	void pressAndReleaseKeyUinputUtf8(const QByteArray& utf8Data);

	static int keysymToLinuxKeycode(uint32_t keysym);
	static int utf8ToLinuxKeycode(const QByteArray& utf8Data);
};
