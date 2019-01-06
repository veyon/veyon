/*
 * NetworkObjectDirectory.cpp - base class for network object directory implementations
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

#include <QTimer>

#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "NetworkObjectDirectory.h"


NetworkObjectDirectory::NetworkObjectDirectory( QObject* parent ) :
	QObject( parent ),
	m_updateTimer( new QTimer( this ) )
{
	connect( m_updateTimer, &QTimer::timeout, this, &NetworkObjectDirectory::update );
}



void NetworkObjectDirectory::setUpdateInterval( int interval )
{
	if( interval >= MinimumUpdateInterval )
	{
		m_updateTimer->start( interval*1000 );
	}
	else
	{
		m_updateTimer->stop();
	}
}
