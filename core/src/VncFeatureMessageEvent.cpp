// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SocketDevice.h"
#include "VncConnection.h"
#include "VncFeatureMessageEvent.h"
#include "rfb/rfbclient.h"


VncFeatureMessageEvent::VncFeatureMessageEvent(const FeatureMessage& featureMessage) :
	m_featureMessage(featureMessage)
{
}



void VncFeatureMessageEvent::fire(rfbClient* client)
{
	vDebug() << qUtf8Printable(QStringLiteral("%1:%2").arg(QString::fromUtf8(client->serverHost)).arg(client->serverPort))
			 << m_featureMessage;

	SocketDevice socketDevice(VncConnection::libvncClientDispatcher, client);

	m_featureMessage.sendAsRfbMessage(&socketDevice);
}
