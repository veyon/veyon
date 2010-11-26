/*
 * DemoServerMaster.h - DemoServerMaster which manages (GUI) slave apps
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 * Copyright (c) 2010 Univention GmbH
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

#ifndef _DEMO_SERVER_MASTER_H
#define _DEMO_SERVER_MASTER_H

#include <QtCore/QStringList>

class ItalcSlaveManager;

class DemoServerMaster
{
public:
	DemoServerMaster( ItalcSlaveManager *slaveManager );
	virtual ~DemoServerMaster();

	void start( int sourcePort, int destinationPort );
	void stop();
	void updateAllowedHosts();

	void allowHost( const QString &host )
	{
		m_allowedHosts += host;
		updateAllowedHosts();
	}

	void unallowHost( const QString &host )
	{
		m_allowedHosts.removeAll( host );
		updateAllowedHosts();
	}

	int serverPort() const
	{
		return m_serverPort;
	}


private:
	ItalcSlaveManager *m_slaveManager;

	QStringList m_allowedHosts;
	int m_serverPort;

} ;

#endif
