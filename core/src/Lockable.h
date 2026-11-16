// SPDX-FileCopyrightText: 2021-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QRecursiveMutex>

class Lockable
{
public:
	virtual void lock()
	{
		m_mutex.lock();
	}

	virtual void unlock()
	{
		m_mutex.unlock();
	}

private:
	QRecursiveMutex m_mutex{};
};
