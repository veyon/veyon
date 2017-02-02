/*
 * Computer.h - represents a computer and provides control methods and data
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

#ifndef COMPUTER_H
#define COMPUTER_H

#include <QString>
#include <QList>

#include "ComputerControlInterface.h"
#include "NetworkObject.h"

class Computer
{
public:
	typedef enum Modes
	{
		ModeMonitoring,
		ModeFullScreenDemo,
		ModeWindowDemo,
		ModeLocked,
		ModeCount
	} Mode;

	typedef enum States
	{
		StateUnreachable,
		StateConnecting,
		StateConnected,
		StateCount
	} State;

	Computer( const Computer& other );
	Computer( NetworkObject::Uid networkObjectUid = 0,
			  const QString& name = QString(),
			  const QString& hostAddress = QString(),
			  const QString& macAddress = QString() );

	bool operator==( const Computer& other ) const
	{
		return name() == other.name() &&
				hostAddress() == other.hostAddress() &&
				macAddress() == other.macAddress();
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

	const ComputerControlInterface& controlInterface() const
	{
		return m_controlInterface;
	}

	ComputerControlInterface& controlInterface()
	{
		return m_controlInterface;
	}

private:
	NetworkObject::Uid m_networkObjectUid;
	QString m_name;
	QString m_hostAddress;
	QString m_macAddress;

	ComputerControlInterface m_controlInterface;

};

typedef QList<Computer> ComputerList;

#endif // COMPUTER_H
