// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "vnckeymap.h"

#include <windows.h>

#include <QElapsedTimer>
#include <QThread>

#include "DesktopInputController.h"
#include "VeyonCore.h"



// wrapper used by vncKeymap
void keybd_uni_event(BYTE bVk, BYTE bScan, DWORD dwFlags, ULONG_PTR dwExtraInfo)
{
	keybd_event(bVk, bScan, dwFlags, dwExtraInfo);
}



DesktopInputController::DesktopInputController(int keyEventInterval) :
	m_keyEventInterval(static_cast<unsigned long>(keyEventInterval))
{
}



DesktopInputController::~DesktopInputController()
{
	m_requestStop = 1;

	if (m_thread.joinable())
	{
		m_thread.join();
	}
}



void DesktopInputController::pressKey(KeyCode key)
{
	m_dataMutex.lock();
	m_keys.enqueue({key, true});
	m_dataMutex.unlock();

	m_inputWaitCondition.wakeAll();
}



void DesktopInputController::releaseKey(KeyCode key)
{
	m_dataMutex.lock();
	m_keys.enqueue({key, false});
	m_dataMutex.unlock();

	m_inputWaitCondition.wakeAll();
}



void DesktopInputController::pressAndReleaseKey(KeyCode key)
{
	pressKey(key);
	releaseKey(key);
}



void DesktopInputController::pressAndReleaseKey(QChar character)
{
	pressAndReleaseKey(static_cast<KeyCode>(character.unicode()));
}



void DesktopInputController::pressAndReleaseKey(QLatin1Char character)
{
	pressAndReleaseKey(static_cast<KeyCode>(character.unicode()));
}



bool DesktopInputController::waitForAllKeysSent(int timeout)
{
	QElapsedTimer timer;
	timer.start();

	// the input thread holds m_dataMutex while sending queued key events,
	// so an empty queue observed under the lock means all keys were sent
	while (timer.elapsed() < timeout)
	{
		m_dataMutex.lock();
		const auto empty = m_keys.isEmpty();
		m_dataMutex.unlock();

		if (empty)
		{
			return true;
		}

		m_inputWaitCondition.wakeAll();
		QThread::msleep(5);
	}

	vWarning() << "timeout while waiting for key events to be sent";
	return false;
}



void DesktopInputController::run()
{
	auto desktop = OpenInputDesktop(0, false, GENERIC_WRITE);
	if (!desktop)
	{
		const auto error = GetLastError();
		vCritical() << "failed to open input desktop:" << error;
		return;
	}

	if (SetThreadDesktop(desktop))
	{
		vncKeymap::ClearShiftKeys();

		QMutex waitMutex;

		while (true)
		{
			m_dataMutex.lock();
			while (m_keys.isEmpty() == false)
			{
				const auto key = m_keys.dequeue();
				vncKeymap::keyEvent(key.first, key.second, false, false);
				QThread::msleep(m_keyEventInterval);
			}
			m_dataMutex.unlock();

			// only stop after the queue has been drained - otherwise key events
			// queued right before destruction (e.g. the final Return of a logon) get lost
			if (m_requestStop)
			{
				QMutexLocker locker(&m_dataMutex);
				if (m_keys.isEmpty())
				{
					break;
				}
				continue;
			}

			waitMutex.lock();
			m_inputWaitCondition.wait(&waitMutex, ThreadSleepInterval);
			waitMutex.unlock();
		}
	}
	else
	{
		const auto error = GetLastError();
		vCritical() << "failed to set thread desktop:" << error;
	}

	CloseDesktop(desktop);
}
