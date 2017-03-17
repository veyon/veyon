/*
 * ItalcCoreConnection.h - declaration of class ItalcCoreConnection
 *
 * Copyright (c) 2008-2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#ifndef ITALC_CORE_CONNECTION_H
#define ITALC_CORE_CONNECTION_H

#include <QPointer>

#include "ItalcCore.h"
#include "ItalcVncConnection.h"

class FeatureMessage;

class ITALC_CORE_EXPORT ItalcCoreConnection : public QObject
{
	Q_OBJECT
public:
	ItalcCoreConnection( ItalcVncConnection *vncConnection );
	~ItalcCoreConnection();

	ItalcVncConnection *vncConnection()
	{
		return m_vncConn;
	}

	ItalcVncConnection::State state() const
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
	static rfbBool handleItalcMessage( rfbClient* client, rfbServerToClientMsg *msg );

	bool handleServerMessage( rfbClient* client, uint8_t msg );


	QPointer<ItalcVncConnection> m_vncConn;

	QString m_user;
	QString m_userHomeDir;

} ;

#endif
