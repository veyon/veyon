/*
 * FeatureMessage.cpp - implementation of a message encapsulation class for features
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QDebug>
#include <QIODevice>

#include "FeatureMessage.h"


bool FeatureMessage::send()
{
	return send( m_ioDevice );
}



bool FeatureMessage::send( QIODevice* ioDevice ) const
{
	if( ioDevice )
	{
		qDebug() << "FeatureMessage::send():" << featureUid() << arguments();

		QDataStream d( ioDevice );
		d << m_featureUid;
		d << m_command;
		d << QVariant( m_arguments );

		return true;
	}

	return false;
}



FeatureMessage &FeatureMessage::receive()
{
	if( m_ioDevice )
	{
		QDataStream d( m_ioDevice );
		QVariant args;
		d >> m_featureUid;
		d >> m_command;
		d >> args;
		m_arguments = args.toMap();
	}

	return *this;
}
