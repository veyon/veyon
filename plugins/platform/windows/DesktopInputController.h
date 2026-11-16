// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

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

	DesktopInputController(int keyEventInterval);
	~DesktopInputController() override;

	void pressKey(KeyCode key);

	void releaseKey(KeyCode key);

	void pressAndReleaseKey(KeyCode key);
	void pressAndReleaseKey(QChar character);
	void pressAndReleaseKey(QLatin1Char character);

private:
	void run();

	static constexpr int ThreadSleepInterval = 100;

	HANDLE m_threadHandle = 0;
	QMutex m_dataMutex;
	QQueue<QPair<KeyCode, bool>> m_keys;
	QWaitCondition m_inputWaitCondition;
	QAtomicInt m_requestStop;

	unsigned long m_keyEventInterval;

	std::thread m_thread = std::thread([this]() { run(); });
};
