/*
 * ItalcCoreServer.h - ItalcCoreServer
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef ITALC_CORE_SERVER_H
#define ITALC_CORE_SERVER_H

#include <QtCore/QMutex>
#include <QtCore/QStringList>

#include "BuiltinFeatures.h"
#include "FeatureManager.h"
#include "FeatureWorkerManager.h"
#include "PluginManager.h"
#include "SocketDevice.h"
#include "ItalcSlaveManager.h"
#include "DesktopAccessPermission.h"

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

	bool handleItalcCoreMessage( SocketDispatcher socketDispatcher, void *user );
	bool handleItalcFeatureMessage( SocketDispatcher socketDispatcher, void *user );

	bool authSecTypeItalc( SocketDispatcher sd, void *user );

	ItalcSlaveManager * slaveManager()
	{
		return &m_slaveManager;
	}

	void setAllowedIPs( const QStringList &allowedIPs );

	bool performAccessControl( const QString& username, const QString& host,
							   DesktopAccessPermission::AuthenticationMethod authenticationMethod );


private:
	void errorMsgAuth( const QString & _ip );

	bool doKeyBasedAuth( SocketDevice &sdev, const QString &host );
	bool doHostBasedAuth( const QString &host );
	bool doCommonSecretAuth( SocketDevice &sdev );

	static ItalcCoreServer *_this;

	QMutex m_dataMutex;
	QStringList m_allowedIPs;

	QStringList m_failedAuthHosts;

	PluginManager m_pluginManager;
	BuiltinFeatures m_builtinFeatures;
	FeatureManager m_featureManager;
	FeatureWorkerManager m_featureWorkerManager;
	ItalcSlaveManager m_slaveManager;

} ;


#endif
