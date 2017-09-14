/*
 * NetworkObjectDirectory.cpp - base class for network object directory implementations
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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
	QObject( parent )
{
	if( VeyonCore::config().networkObjectDirectoryUpdateInterval() >= MinimumUpdateInterval )
	{
		// create and start directory update timer
		auto t = new QTimer( this );
		connect( t, &QTimer::timeout, this, &NetworkObjectDirectory::update );
		t->start( VeyonCore::config().networkObjectDirectoryUpdateInterval() * 1000 );
	}
}
