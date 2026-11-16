// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ProcessHelper.h"


ProcessHelper::ProcessHelper(const QString& program, const QStringList& arguments) :
	m_process()
{
	m_process.start(program, arguments);
}



int ProcessHelper::run()
{
	if (m_process.waitForStarted())
	{
		m_process.waitForFinished();
		return m_process.exitCode();
	}

	return -1;
}



QByteArray ProcessHelper::runAndReadAll()
{
	if (m_process.waitForStarted())
	{
		m_process.waitForFinished();
		return m_process.readAll();
	}

	return QByteArray();
}
