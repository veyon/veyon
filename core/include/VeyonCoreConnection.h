/*
 * VeyonCoreConnection.h - declaration of class VeyonCoreConnection
 *
 * Copyright (c) 2008-2016 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef VEYON_CORE_CONNECTION_H
#define VEYON_CORE_CONNECTION_H

#include <QPointer>

#include "VeyonCore.h"
#include "VeyonVncConnection.h"

class FeatureMessage;

class VEYON_CORE_EXPORT VeyonCoreConnection : public QObject
{
	Q_OBJECT
public:
	VeyonCoreConnection( VeyonVncConnection *vncConnection );
	~VeyonCoreConnection() override;

	VeyonVncConnection *vncConnection()
	{
		return m_vncConn;
	}

	VeyonVncConnection::State state() const
	{
		return m_vncConn->state();
	}

	bool isConnected() const
	{
		return m_vncConn && m_vncConn->isConnected();
	}

	const QString & user() const
	{
		return m_user;
	}

	const QString & userHomeDir() const
	{
		return m_userHomeDir;
	}

	void sendFeatureMessage( const FeatureMessage &featureMessage );

signals:
	void featureMessageReceived( const FeatureMessage& );

private slots:
	void initNewClient( rfbClient *client );

private:
	static rfbBool handleVeyonMessage( rfbClient* client, rfbServerToClientMsg *msg );

	bool handleServerMessage( rfbClient* client, uint8_t msg );


	QPointer<VeyonVncConnection> m_vncConn;

	QString m_user;
	QString m_userHomeDir;

} ;

#endif
