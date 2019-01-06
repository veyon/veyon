/*
 * DemoServerProtocol.h - header file for DemoServerProtocol class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef DEMO_SERVER_PROTOCOL_H
#define DEMO_SERVER_PROTOCOL_H

#include "VncServerClient.h"
#include "VncServerProtocol.h"

// clazy:excludeall=copyable-polymorphic

class DemoServerProtocol : public VncServerProtocol
{
public:
	DemoServerProtocol( const QString& demoAccessToken, QTcpSocket* socket, VncServerClient* client );

protected:
	QVector<RfbVeyonAuth::Type> supportedAuthTypes() const override;
	void processAuthenticationMessage( VariantArrayMessage& message ) override;
	void performAccessControl() override;

private:
	VncServerClient::AuthState performTokenAuthentication( VariantArrayMessage& message );

	const QString m_demoAccessToken;

} ;

#endif
