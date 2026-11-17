// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>

#include <QDeadlineTimer>
#include <QLocalSocket>

#include "IpcConnection.h"


QLocalSocket* IpcConnection::connectToEndpoint(const QString& directory, const QString& endpointName, int timeout)
{
	QDeadlineTimer deadline(timeout);

	Q_UNUSED(directory)

	const QString pipeName = QStringLiteral("\\\\.\\pipe\\") + endpointName;
	const auto pipeNameWide = reinterpret_cast<LPCWSTR>(pipeName.utf16());

	while (true)
	{
		const auto handle = CreateFileW(pipeNameWide, GENERIC_READ | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES, 0,
										nullptr, OPEN_EXISTING,
										FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr);
		if (handle != INVALID_HANDLE_VALUE)
		{
			auto socket = new QLocalSocket;
			if (socket->setSocketDescriptor(reinterpret_cast<qintptr>(handle)))
			{
				return socket;
			}
			CloseHandle(handle);
			delete socket;
			return nullptr;
		}

		const auto error = GetLastError();
		if (error != ERROR_PIPE_BUSY || deadline.hasExpired())
		{
			vCritical() << "could not connect to" << endpointName << error;
			return nullptr;
		}

		WaitNamedPipeW(pipeNameWide, DWORD(qMax<qint64>(1, deadline.remainingTime())));
	}
}
