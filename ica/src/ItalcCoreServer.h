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

#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QPair>
#include <QtCore/QSignalMapper>
#include <QtCore/QStringList>

#include "ItalcCore.h"
#include "MasterProcess.h"


class ItalcCoreServer : public QObject
{
	Q_OBJECT
public:
	ItalcCoreServer();
	virtual ~ItalcCoreServer();

	static ItalcCoreServer * instance()
	{
		Q_ASSERT( this != NULL );
		return _this;
	}

	int handleItalcClientMessage( socketDispatcher sd, void *user );

	bool authSecTypeItalc( socketDispatcher sd, void *user );

	MasterProcess * masterProcess()
	{
		return &m_masterProcess;
	}

	void setAllowedIPs( const QStringList &allowedIPs )
	{
		m_allowedIPs = allowedIPs;
	}


private:
	static void errorMsgAuth( const QString & _ip );

	bool doKeyBasedAuth( SocketDevice &sdev );
	bool doHostBasedAuth( const QString &host );

	static ItalcCoreServer * _this;

	QStringList m_allowedIPs;

	MasterProcess m_masterProcess;

} ;


#endif
