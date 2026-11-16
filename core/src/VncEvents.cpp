// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "VncEvents.h"

#include <rfb/rfbclient.h>


VncKeyEvent::VncKeyEvent(KeyCode key, bool pressed) :
	m_key(key),
	m_pressed(pressed)
{
}


void VncKeyEvent::fire(rfbClient* client)
{
	SendKeyEvent(client, m_key, m_pressed ? TRUE : FALSE);
}



VncPointerEvent::VncPointerEvent(int x, int y, int buttonMask) :
	m_x(x),
	m_y(y),
	m_buttonMask(buttonMask)
{
}



void VncPointerEvent::fire(rfbClient* client)
{
	SendPointerEvent(client, m_x, m_y, m_buttonMask);
}



VncClientCutEvent::VncClientCutEvent(const QString& text) :
	m_text(text.toUtf8())
{
}



void VncClientCutEvent::fire(rfbClient* client)
{
	SendClientCutText(client, m_text.data(), m_text.size()); // clazy:exclude=detaching-member
}



void VncUpdateFormatAndEncodingsEvent::fire(rfbClient* client)
{
	SetFormatAndEncodings(client);
}



void VncFullFramebufferUpdateRequestEvent::fire(rfbClient* client)
{
	SendFramebufferUpdateRequest(client, 0, 0, client->width, client->height, FALSE);
}
