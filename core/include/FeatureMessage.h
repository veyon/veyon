/*
 * FeatureMessage.h - header for a message encapsulation class for features
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

#ifndef FEATURE_MESSAGE_H
#define FEATURE_MESSAGE_H

#include <QVariant>

#include "Feature.h"

class QIODevice;

class VEYON_CORE_EXPORT FeatureMessage
{
public:
	typedef quint32 MessageSize;
	typedef Feature::Uid FeatureUid;
	typedef qint32 Command;
	typedef QMap<QString, QVariant> Arguments;

	enum SpecialCommands
	{
		DefaultCommand = 0,
		InvalidCommand = -1,
		InitCommand = -2,
	};

	explicit FeatureMessage( QIODevice* ioDevice = nullptr ) :
		m_ioDevice( ioDevice ),
		m_featureUid(),
		m_command( InvalidCommand ),
		m_arguments()
	{
	}

	explicit FeatureMessage( FeatureUid featureUid, Command command ) :
		m_ioDevice( nullptr ),
		m_featureUid( featureUid ),
		m_command( command ),
		m_arguments()
	{
	}

	explicit FeatureMessage( const FeatureMessage& other ) :
		m_ioDevice( other.ioDevice() ),
		m_featureUid( other.featureUid() ),
		m_command( other.command() ),
		m_arguments( other.arguments() )
	{
	}

	~FeatureMessage() {}

	FeatureMessage& operator=( const FeatureMessage& other )
	{
		m_ioDevice = other.ioDevice();
		m_featureUid = other.featureUid();
		m_command = other.command();
		m_arguments = other.arguments();

		return *this;
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

	bool hasArgument( int index ) const
	{
		return m_arguments.contains( QString::number( index ) );
	}

	bool send();
	bool send( QIODevice* ioDevice ) const;

	bool isReadyForReceive();

	bool receive();

	QIODevice* ioDevice() const
	{
		return m_ioDevice;
	}

private:
	QIODevice* m_ioDevice;

	FeatureUid m_featureUid;
	Command m_command;
	Arguments m_arguments;

} ;

#endif // FEATURE_MESSAGE_H
