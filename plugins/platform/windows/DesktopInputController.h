/*
 * DesktopInputController.h - declaration of DesktopInputController class
 *
 * Copyright (c) 2019-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <thread>

#include <QMutex>
#include <QQueue>
#include <QWaitCondition>

// clazy:excludeall=copyable-polymorphic

class DesktopInputController : public QObject
{
	Q_OBJECT
public:
	using KeyCode = uint32_t;

	DesktopInputController( int keyEventInterval );
	~DesktopInputController() override;

	void pressKey( KeyCode key );

	void releaseKey( KeyCode key );

	void pressAndReleaseKey( KeyCode key );
	void pressAndReleaseKey( QChar character );
	void pressAndReleaseKey( QLatin1Char character );

private:
	void run();

	static constexpr int ThreadSleepInterval = 100;

	HANDLE m_threadHandle = 0;
	QMutex m_dataMutex;
	QQueue<QPair<KeyCode, bool> > m_keys;
	QWaitCondition m_inputWaitCondition;
	QAtomicInt m_requestStop;

	unsigned long m_keyEventInterval;

	std::thread m_thread = std::thread([this](){ run(); });

};
