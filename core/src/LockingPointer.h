// SPDX-FileCopyrightText: 2021-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

template<class T>
class LockingPointer
{
public:
	LockingPointer(T lockable) :
		m_lockable(lockable)
	{
		if (m_lockable)
		{
			m_lockable->lock();
		}
	}

	~LockingPointer()
	{
		if (m_lockable)
		{
			m_lockable->unlock();
		}
	}

	LockingPointer(LockingPointer<T>&& lockingPointer) :
		m_lockable(lockingPointer.m_lockable)
	{
		lockingPointer.m_lockable = nullptr;
	}

	LockingPointer(LockingPointer<T> const&) = delete;
	LockingPointer& operator=(LockingPointer<T> const&) = delete;

	T operator->() const
	{
		return m_lockable;
	}

	explicit operator bool() const
	{
		return static_cast<bool>(m_lockable);
	}

private:
	T m_lockable;
};
