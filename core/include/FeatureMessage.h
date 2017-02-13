/*
 * FeatureMessage.h - header for a message encapsulation class for features
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

#ifndef FEATURE_MESSAGE_H
#define FEATURE_MESSAGE_H

#include <QVariant>

#include "Feature.h"

class QIODevice;

class FeatureMessage
{
public:
	typedef Feature::Uid FeatureUid;
	typedef qint32 Command;
	typedef QMap<QString, QVariant> Arguments;

	FeatureMessage( QIODevice* ioDevice = nullptr, const FeatureUid& featureUid = Feature::Uid(), Command command = -1 ) :
		m_ioDevice( ioDevice ),
		m_featureUid( featureUid ),
		m_command( command ),
		m_arguments()
	{
	}

	FeatureMessage( const FeatureUid &featureUid, Command command = -1 ) :
		m_ioDevice( nullptr ),
		m_featureUid( featureUid ),
		m_command( command ),
		m_arguments()
	{
	}

	const FeatureUid& featureUid() const
	{
		return m_featureUid;
	}

	Command command() const
	{
		return m_command;
	}

	const Arguments& arguments() const
	{
		return m_arguments;
	}

	FeatureMessage &addArgument( int index, const QVariant& value )
	{
		m_arguments[QString::number(index)] = value;
		return *this;
	}

	QVariant argument( int index ) const
	{
		return m_arguments[QString::number(index)];
	}

	bool send();
	bool send( QIODevice* ioDevice ) const;

	FeatureMessage& receive();


private:
	QIODevice* m_ioDevice;

	FeatureUid m_featureUid;
	Command m_command;
	Arguments m_arguments;

} ;

#endif // FEATURE_MESSAGE_H
