/*
 * Computer.h - represents a computer and provides control methods and data
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#pragma once

#include <QString>
#include <QVector>

#include "NetworkObject.h"

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT Computer
{
public:
	explicit Computer( NetworkObject::Uid networkObjectUid = NetworkObject::Uid(),
					   const QString& name = {},
					   const QString& hostAddress = {},
					   const QString& macAddress = {},
					   const QString& location = {} );

	bool operator==( const Computer& other ) const
	{
		return networkObjectUid() == other.networkObjectUid();
	}

	bool operator!=( const Computer& other ) const
	{
		return !(*this == other);
	}

	NetworkObject::Uid networkObjectUid() const
	{
		return m_networkObjectUid;
	}

	void setName( const QString& name )
	{
		m_name = name;
	}

	QString name() const
	{
		return m_name;
	}

	void setHostAddress( const QString& hostAddress )
	{
		m_hostAddress = hostAddress;
	}

	QString hostAddress() const
	{
		return m_hostAddress;
	}

	void setMacAddress( const QString& macAddress )
	{
		m_macAddress = macAddress;
	}

	QString macAddress() const
	{
		return m_macAddress;
	}

	void setLocation( const QString& location )
	{
		m_location = location;
	}

	const QString& location() const
	{
		return m_location;
	}

private:
	NetworkObject::Uid m_networkObjectUid;
	QString m_name;
	QString m_hostAddress;
	QString m_macAddress;
	QString m_location;

};

using ComputerList = QVector<Computer>;
