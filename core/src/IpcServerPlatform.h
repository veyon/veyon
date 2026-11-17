// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IpcServer.h"

// platform part of IpcServer (internal, not installed): the listening endpoint, implemented by
// IpcServer::UnixListener (IpcServerUnix.cpp, QLocalServer) and IpcServer::WindowsListener
// (IpcServerWindows.cpp, named pipe); core/CMakeLists.txt selects the file. Every accepted
// connection is passed to IpcServer::acceptConnection().
class IpcServer::Listener
{
public:
	virtual ~Listener() = default;

	static std::unique_ptr<Listener> create(IpcServer* server);

	virtual bool listen(const QString& directory) = 0;
	virtual void close() = 0;

	// called after a connection was dropped or a handshake failed; false if the child could not
	// connect again (Windows: no listening pipe instance)
	virtual bool ensureListener() = 0;
};
