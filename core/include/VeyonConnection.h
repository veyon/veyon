/*
 * VeyonConnection.h - declaration of class VeyonConnection
 *
 * Copyright (c) 2008-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QPointer>

#include "RfbVeyonAuth.h"
#include "VncConnection.h"


class FeatureMessage;

class VEYON_CORE_EXPORT VeyonConnection : public QObject
{
	Q_OBJECT
public:
	VeyonConnection( VncConnection* vncConnection );
	~VeyonConnection() override;

	VncConnection* vncConnection()
	{
		return m_vncConnection;
	}

	VncConnection::State state() const
	{
		return m_vncConnection->state();
	}

	bool isConnected() const
	{
		return m_vncConnection && m_vncConnection->isConnected();
	}

	const QString& user() const
	{
		return m_user;
	}

	const QString& userHomeDir() const
	{
		return m_userHomeDir;
	}

	void setVeyonAuthType( RfbVeyonAuth::Type authType )
	{
		m_veyonAuthType = authType;
	}

	RfbVeyonAuth::Type veyonAuthType() const
	{
		return m_veyonAuthType;
	}

	void sendFeatureMessage( const FeatureMessage& featureMessage, bool wake );

	bool handleServerMessage( rfbClient* client, uint8_t msg );

	static constexpr auto VeyonConnectionTag = 0xFE14A11;


signals:
	void featureMessageReceived( const FeatureMessage& );

private:
	void registerConnection();
	void unregisterConnection();

	// authentication
	static int8_t handleSecTypeVeyon( rfbClient* client, uint32_t authScheme );
	static void hookPrepareAuthentication( rfbClient* client );

	QPointer<VncConnection> m_vncConnection;

	RfbVeyonAuth::Type m_veyonAuthType;

	QString m_user;
	QString m_userHomeDir;

} ;
