/*
 * VncProxyConnectionFactory.h - abstract factory class for VncProxyConnectionFactory objects
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include "CryptoCore.h"

class QTcpSocket;
class VncProxyConnection;

// clazy:excludeall=copyable-polymorphic

class VncProxyConnectionFactory
{
public:
	using Password = CryptoCore::SecureArray;

	virtual ~VncProxyConnectionFactory() = default;

	virtual VncProxyConnection* createVncProxyConnection( QTcpSocket* clientSocket,
														  int vncServerPort,
														  const Password& vncServerPassword,
														  QObject* parent ) = 0;

} ;
