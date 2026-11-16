// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>

#include "WindowsKeyboardShortcutTrapper.h"

// clazy:excludeall=non-pod-global-static

static QMutex __trappedShortcutsMutex;
static QList<WindowsKeyboardShortcutTrapper::Shortcut> __trappedShortcuts;

QMutex WindowsKeyboardShortcutTrapper::s_refCntMutex;
int WindowsKeyboardShortcutTrapper::s_refCnt = 0;


static HHOOK __lowLevelKeyboardHookHandle = nullptr; // hook handle


LRESULT CALLBACK TaskKeyHookLL(int nCode, WPARAM wp, LPARAM lp)
{
	auto pkh = reinterpret_cast<KBDLLHOOKSTRUCT*>(static_cast<intptr_t>(lp));
	static QList<KeyboardShortcutTrapper::Shortcut> pressed;

	if (nCode == HC_ACTION)
	{
		BOOL isCtrlKeyDown = GetAsyncKeyState(VK_CONTROL) >> ((sizeof(SHORT) * 8) - 1);
		QMutexLocker m(&__trappedShortcutsMutex);

		KeyboardShortcutTrapper::Shortcut shortcut = KeyboardShortcutTrapper::NoShortcut;

		if (pkh->vkCode == VK_ESCAPE && isCtrlKeyDown)
		{
			shortcut = KeyboardShortcutTrapper::CtrlEsc;
		}
		else if (pkh->vkCode == VK_TAB && pkh->flags & LLKHF_ALTDOWN)
		{
			shortcut = KeyboardShortcutTrapper::AltTab;
		}
		else if (pkh->vkCode == VK_ESCAPE && pkh->flags & LLKHF_ALTDOWN)
		{
			shortcut = KeyboardShortcutTrapper::AltEsc;
		}
		else if (pkh->vkCode == VK_SPACE && pkh->flags & LLKHF_ALTDOWN)
		{
			shortcut = KeyboardShortcutTrapper::AltSpace;
		}
		else if (pkh->vkCode == VK_F4 && pkh->flags & LLKHF_ALTDOWN)
		{
			shortcut = KeyboardShortcutTrapper::AltF4;
		}
		else if (pkh->vkCode == VK_LWIN || pkh->vkCode == VK_RWIN)
		{
			pressed.removeAll(KeyboardShortcutTrapper::SuperKeyDown);
			pressed.removeAll(KeyboardShortcutTrapper::SuperKeyUp);
			if (wp == WM_KEYDOWN)
			{
				shortcut = KeyboardShortcutTrapper::SuperKeyDown;
			}
			else
			{
				shortcut = KeyboardShortcutTrapper::SuperKeyUp;
			}
		}

		if (shortcut != KeyboardShortcutTrapper::NoShortcut)
		{
			if (pressed.contains(shortcut) == false)
			{
				__trappedShortcuts.push_back(shortcut);
				pressed << shortcut;
			}
			else
			{
				pressed.removeAll(shortcut);
			}
			return 1;
		}
	}
	return CallNextHookEx(__lowLevelKeyboardHookHandle, nCode, wp, lp);
}



WindowsKeyboardShortcutTrapper::WindowsKeyboardShortcutTrapper(QObject* parent) :
	KeyboardShortcutTrapper(parent),
	m_pollTimer(this)
{
	connect(&m_pollTimer, &QTimer::timeout,
			this, &WindowsKeyboardShortcutTrapper::forwardTrappedShortcuts);
}



WindowsKeyboardShortcutTrapper::~WindowsKeyboardShortcutTrapper()
{
	setEnabled(false);
}



void WindowsKeyboardShortcutTrapper::setEnabled(bool on)
{
	if (on == m_enabled)
	{
		return;
	}

	s_refCntMutex.lock();

	m_enabled = on;
	if (on)
	{
		if (!s_refCnt)
		{
			if (__lowLevelKeyboardHookHandle == nullptr)
			{
				HINSTANCE hAppInstance = GetModuleHandle(nullptr);
				// set lowlevel-keyboard-hook
				__lowLevelKeyboardHookHandle = SetWindowsHookEx(WH_KEYBOARD_LL, TaskKeyHookLL, hAppInstance, 0);
			}
		}

		m_pollTimer.start(PollTimerInterval);
		++s_refCnt;
	}
	else
	{
		--s_refCnt;
		if (!s_refCnt)
		{
			UnhookWindowsHookEx(__lowLevelKeyboardHookHandle);
			__lowLevelKeyboardHookHandle = nullptr;
			m_pollTimer.stop();
		}
	}
	s_refCntMutex.unlock();
}



void WindowsKeyboardShortcutTrapper::forwardTrappedShortcuts()
{
	QMutexLocker m(&__trappedShortcutsMutex);

	while (__trappedShortcuts.isEmpty() == false)
	{
		Q_EMIT shortcutTrapped(__trappedShortcuts.takeFirst());
	}
}
