/*
 *  LockingPointer.h - smart pointer for lockables
 *
 *  Copyright (c) 2021 Tobias Junghans <tobydox@veyon.io>
 *
 *  This file is part of Veyon - https://veyon.io
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#pragma once

template<class T>
class LockingPointer
{
public:
	LockingPointer( T lockable ) : m_lockable( lockable )
	{
		if( m_lockable )
		{
			m_lockable->lock();
		}
	}

	~LockingPointer()
	{
		if( m_lockable )
		{
			m_lockable->unlock();
		}
	}

	LockingPointer( LockingPointer<T>&& lockingPointer ) : m_lockable( lockingPointer.m_lockable )
	{
		lockingPointer.m_lockable = nullptr;
	}

	LockingPointer( LockingPointer<T> const& ) = delete;
	LockingPointer& operator=( LockingPointer<T> const& ) = delete;

	T operator->() const
	{
		return m_lockable;
	}

private:
	T m_lockable;
};

