// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

using rfbClient = struct _rfbClient;

// clazy:excludeall=copyable-polymorphic

class VncEvent
{
public:
	virtual ~VncEvent() = default;
	virtual void fire(rfbClient* client) = 0;
};


class VncKeyEvent : public VncEvent
{
public:
	using KeyCode = unsigned int;

	VncKeyEvent(KeyCode key, bool pressed);

	void fire(rfbClient* client) override;

private:
	KeyCode m_key;
	bool m_pressed;
};


class VncPointerEvent : public VncEvent
{
public:
	VncPointerEvent(int x, int y, int buttonMask);

	void fire(rfbClient* client) override;

private:
	int m_x;
	int m_y;
	int m_buttonMask;
};


class VncClientCutEvent : public VncEvent
{
public:
	explicit VncClientCutEvent(const QString& text);

	void fire(rfbClient* client) override;

private:
	QByteArray m_text;
};

class VncUpdateFormatAndEncodingsEvent : public VncEvent
{
public:
	void fire(rfbClient* client) override;
};

class VncFullFramebufferUpdateRequestEvent : public VncEvent
{
public:
	void fire(rfbClient* client) override;
};
