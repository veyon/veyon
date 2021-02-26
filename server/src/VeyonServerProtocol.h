/*
 * VeyonServerProtocol.h - header file for the VeyonServerProtocol class
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include "VncServerProtocol.h"

class ServerAuthenticationManager;
class ServerAccessControlManager;

// clazy:excludeall=copyable-polymorphic

class VeyonServerProtocol : public VncServerProtocol
{
public:
	VeyonServerProtocol( QTcpSocket* socket,
						  VncServerClient* client,
						  ServerAuthenticationManager& serverAuthenticationManager,
						  ServerAccessControlManager& serverAccessControlManager );

protected:
	QVector<RfbVeyonAuth::Type> supportedAuthTypes() const override;
	void processAuthenticationMessage( VariantArrayMessage& message ) override;
	void performAccessControl() override;

private:
	ServerAuthenticationManager& m_serverAuthenticationManager;
	ServerAccessControlManager& m_serverAccessControlManager;

} ;
