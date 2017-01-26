/*
 * MasterCore.h - global instances
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

#ifndef MASTER_CORE_H
#define MASTER_CORE_H

class ItalcVncConnection;
class ItalcCoreConnection;
class ComputerManager;
class PersonalConfig;

class MasterCore
{
public:
	static MasterCore& i()
	{
		if( s_instance == Q_NULLPTR )
		{
			s_instance = new MasterCore();
		}

		return *s_instance;
	}

	void shutdown();

	ItalcVncConnection& localDisplay()
	{
		return *m_localDisplay;
	}

	ItalcCoreConnection& localService()
	{
		return *m_localService;
	}

	PersonalConfig& personalConfig()
	{
		return *m_personalConfig;
	}

	ComputerManager& computerManager()
	{
		return *m_computerManager;
	}


private:
	static MasterCore* s_instance;

	MasterCore();
	~MasterCore();

	ItalcVncConnection* m_localDisplay;
	ItalcCoreConnection* m_localService;
	PersonalConfig* m_personalConfig;
	ComputerManager* m_computerManager;

} ;

#endif
