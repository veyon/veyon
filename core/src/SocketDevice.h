// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QIODevice>

class SocketDevice : public QIODevice
{
	Q_OBJECT
public:
	enum SocketOperation {
		SocketOpRead,
		SocketOpWrite
	};

	using Dispatcher = qint64 (*)(char*, const qint64, SocketOperation, void*);

	explicit SocketDevice(Dispatcher dispatcher, void* user = nullptr, QObject* parent = nullptr) :
		QIODevice(parent),
		m_dispatcher(dispatcher),
		m_user(user)
	{
		open(ReadWrite | Unbuffered); // Flawfinder: ignore
	}

	Dispatcher dispatcher()
	{
		return m_dispatcher;
	}

	void* user()
	{
		return m_user;
	}

	void setUser(void* user)
	{
		m_user = user;
	}

	qint64 read(char* buf, qint64 bytes) // Flawfinder: ignore
	{
		return readData(buf, bytes);
	}

	qint64 write(const char* buf, qint64 bytes)
	{
		return writeData(buf, bytes);
	}

protected:
	qint64 readData(char* buf, qint64 bytes) override
	{
		return m_dispatcher(buf, bytes, SocketOpRead, m_user);
	}

	qint64 writeData(const char* buf, qint64 bytes) override
	{
		return m_dispatcher(const_cast<char*>(buf), bytes,
							SocketOpWrite, m_user);
	}


private:
	Dispatcher m_dispatcher;
	void* m_user;
};
