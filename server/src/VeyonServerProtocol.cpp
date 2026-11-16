// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QTcpSocket>

#include "ServerAccessControlManager.h"
#include "ServerAuthenticationManager.h"
#include "VeyonServerProtocol.h"


VeyonServerProtocol::VeyonServerProtocol(QTcpSocket* socket,
										 VncServerClient* client,
										 ServerAuthenticationManager& serverAuthenticationManager,
										 ServerAccessControlManager& serverAccessControlManager) :
	VncServerProtocol(socket, client),
	m_serverAuthenticationManager(serverAuthenticationManager),
	m_serverAccessControlManager(serverAccessControlManager)
{
}



QVector<RfbVeyonAuth::Type> VeyonServerProtocol::supportedAuthTypes() const
{
	return m_serverAuthenticationManager.supportedAuthTypes();
}



void VeyonServerProtocol::processAuthenticationMessage(VariantArrayMessage& message)
{
	m_serverAuthenticationManager.processAuthenticationMessage(client(), message);
}



void VeyonServerProtocol::performAccessControl()
{
	// perform access control via ServerAccessControl manager if either
	// client just entered access control or is still waiting to be
	// processed (e.g. desktop access dialog already active for a different connection)
	if (client()->accessControlState() == VncServerClient::AccessControlState::Init ||
		client()->accessControlState() == VncServerClient::AccessControlState::Waiting)
	{
		m_serverAccessControlManager.addClient(client());
	}
}
