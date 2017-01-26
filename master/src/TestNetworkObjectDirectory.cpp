/*
 * TestNetworkObjectDirectory.cpp - provides a NetworkObjectDirectory implementation for testing
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QTimer>

#include "TestNetworkObjectDirectory.h"

TestNetworkObjectDirectory::TestNetworkObjectDirectory(  QObject* parent ) :
	NetworkObjectDirectory( parent )
{
	NetworkObject lab1( NetworkObject::Group, "Lab 1" );
	NetworkObject lab2( NetworkObject::Group, "Lab 2" );
	NetworkObject lab3( NetworkObject::Group, "Lab 3" );

	m_objects[lab1] += NetworkObject( NetworkObject::Host, "Computer B" );
	m_objects[lab1] += NetworkObject( NetworkObject::Host, "Computer C" );
	m_objects[lab1] += NetworkObject( NetworkObject::Host, "Computer D" );
	m_objects[lab1] += NetworkObject( NetworkObject::Host, "Computer A" );
	m_objects[lab2] += NetworkObject( NetworkObject::Host, "Computer X" );
	m_objects[lab2] += NetworkObject( NetworkObject::Host, "Computer Y" );
	m_objects[lab2] += NetworkObject( NetworkObject::Host, "Computer Z" );
	m_objects[lab3] = QList<NetworkObject>();

	QTimer*t = new QTimer( this );
	connect( t, &QTimer::timeout, this, &TestNetworkObjectDirectory::test );
	t->start(2000);
}



QList<NetworkObject> TestNetworkObjectDirectory::objects(const NetworkObject &parent)
{
	if( parent.type() == NetworkObject::Root )
	{
		return m_objects.keys();
	}
	else if( parent.type() == NetworkObject::Group &&
			 m_objects.contains( parent ) )
	{
		return m_objects[parent];
	}

	return QList<NetworkObject>();
}



void TestNetworkObjectDirectory::test()
{
	NetworkObject lab1( NetworkObject::Group, "Lab 1" );

	emit objectsAboutToBeInserted( lab1, m_objects[lab1].count(), 1 );

	static int id = 0;
	m_objects[lab1] += NetworkObject( NetworkObject::Host, QString( "Computer %1" ).arg( ++id ) );

	emit objectsInserted();

	emit objectsAboutToBeRemoved( lab1, 0, 1 );
	m_objects[lab1].takeFirst();
	emit objectsRemoved();
}
