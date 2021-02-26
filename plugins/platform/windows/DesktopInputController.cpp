/*
 * DesktopInputController.cpp - implementation of DesktopInputController class
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "vnckeymap.h"

#include <windows.h>

#include <QElapsedTimer>
#include <QTimer>

#include "DesktopInputController.h"
#include "VeyonCore.h"



// wrapper used by vncKeymap
void keybd_uni_event( BYTE bVk, BYTE bScan, DWORD dwFlags, ULONG_PTR dwExtraInfo )
{
	keybd_event( bVk, bScan, dwFlags, dwExtraInfo );
}



DesktopInputController::DesktopInputController( int keyEventInterval ) :
	m_keyEventInterval( static_cast<unsigned long>( keyEventInterval ) )
{
	start();
}



DesktopInputController::~DesktopInputController()
{
	requestInterruption();
	wait( ThreadStopTimeout );
}



void DesktopInputController::pressKey( KeyCode key )
{
	m_dataMutex.lock();
	m_keys.enqueue( { key, true } );
	m_dataMutex.unlock();

	m_inputWaitCondition.wakeAll();
}



void DesktopInputController::releaseKey( KeyCode key )
{
	m_dataMutex.lock();
	m_keys.enqueue( { key, false } );
	m_dataMutex.unlock();

	m_inputWaitCondition.wakeAll();
}



void DesktopInputController::pressAndReleaseKey( KeyCode key )
{
	pressKey( key );
	releaseKey( key );
}



void DesktopInputController::pressAndReleaseKey( QChar character )
{
	pressAndReleaseKey( static_cast<KeyCode>( character.unicode() ) );
}



void DesktopInputController::pressAndReleaseKey( QLatin1Char character )
{
	pressAndReleaseKey( static_cast<KeyCode>( character.unicode() ) );
}



void DesktopInputController::run()
{
	auto desktop = OpenInputDesktop( 0, false, GENERIC_WRITE );

	if( SetThreadDesktop( desktop ) )
	{
		vncKeymap::ClearShiftKeys();

		QMutex waitMutex;

		while( isInterruptionRequested() == false )
		{
			waitMutex.lock();
			m_inputWaitCondition.wait( &waitMutex, QDeadlineTimer( ThreadSleepInterval ) );
			waitMutex.unlock();

			m_dataMutex.lock();
			while( m_keys.isEmpty() == false )
			{
				const auto key = m_keys.dequeue();
				vncKeymap::keyEvent( key.first, key.second, false, false );
				QThread::msleep( m_keyEventInterval );
			}
			m_dataMutex.unlock();
		}
	}
	else
	{
		vCritical() << GetLastError();
	}

	CloseDesktop( desktop );
}
