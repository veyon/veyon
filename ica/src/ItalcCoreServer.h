/*
 * ItalcCoreServer.h - ItalcCoreServer
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _ITALC_CORE_SERVER_H
#define _ITALC_CORE_SERVER_H

#include <QtCore/QMutex>
#include <QtCore/QStringList>

#include "SocketDevice.h"
#include "ItalcSlaveManager.h"


class ItalcCoreServer : public QObject
{
	Q_OBJECT
public:
	ItalcCoreServer();
	virtual ~ItalcCoreServer();

	static ItalcCoreServer *instance()
	{
		Q_ASSERT( _this != NULL );
		return _this;
	}

	int handleItalcClientMessage( socketDispatcher sd, void *user );

	bool authSecTypeItalc( socketDispatcher sd, void *user );

	ItalcSlaveManager * slaveManager()
	{
		return &m_slaveManager;
	}

	void setAllowedIPs( const QStringList &allowedIPs )
	{
		QMutexLocker l( &m_dataMutex );
		m_allowedIPs = allowedIPs;
	}


private:
	static void errorMsgAuth( const QString & _ip );

	bool doKeyBasedAuth( SocketDevice &sdev, const QString &host );
	bool doHostBasedAuth( const QString &host );
	bool doCommonSecretAuth( SocketDevice &sdev );

	static ItalcCoreServer *_this;

	QMutex m_dataMutex;
	QStringList m_allowedIPs;

	// list of hosts that are allowed/denied to access ICA when ICA is running
	// under a role different from "RoleOther"
	QStringList m_manuallyAllowedHosts;
	QStringList m_manuallyDeniedHosts;

	ItalcSlaveManager m_slaveManager;

} ;


#endif
