/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "FdChannel.h"

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

FdChannel::FdChannel(int readFd, int writeFd, QObject* parent) :
	QObject(parent), m_readFd(readFd), m_writeFd(writeFd)
{
	if (readFd < 0 || writeFd < 0 ||
		fcntl(readFd, F_SETFD, FD_CLOEXEC) < 0 || fcntl(writeFd, F_SETFD, FD_CLOEXEC) < 0)
	{
		return;
	}
	const auto readFlags = fcntl(readFd, F_GETFL);
	const auto writeFlags = fcntl(writeFd, F_GETFL);
	if (readFlags < 0 || writeFlags < 0 || fcntl(readFd, F_SETFL, readFlags | O_NONBLOCK) < 0 ||
		fcntl(writeFd, F_SETFL, writeFlags | O_NONBLOCK) < 0)
	{
		return;
	}
	m_valid = true;
	m_reader = new QSocketNotifier(readFd, QSocketNotifier::Read, this);
	m_writer = new QSocketNotifier(writeFd, QSocketNotifier::Write, this);
	m_writer->setEnabled(false);
	connect(m_reader, &QSocketNotifier::activated, this, [this]() { read(); });
	connect(m_writer, &QSocketNotifier::activated, this, [this]() { flush(); });
}

FdChannel::~FdChannel()
{
	if (m_readFd >= 0)
	{
		::close(m_readFd);
	}
	if (m_writeFd >= 0 && m_writeFd != m_readFd)
	{
		::close(m_writeFd);
	}
}

void FdChannel::send(const QByteArray& data)
{
	if (!m_valid)
	{
		return;
	}
	if (m_output.size() + data.size() > 2 * 1024 * 1024)
	{
		fail();
		return;
	}
	m_output += data;
	flush();
}

void FdChannel::read()
{
	char buffer[8192];
	// Yield regularly so a chatty UI cannot starve D-Bus and login deadlines.
	for (int batch = 0; m_valid && batch < 64; ++batch)
	{
		const auto size = ::read(m_readFd, buffer, sizeof(buffer));
		if (size > 0)
		{
			Q_EMIT received(QByteArray(buffer, int(size)));
		}
		else if (size < 0 && errno == EINTR)
		{
			continue;
		}
		else if (size < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
		{
			return;
		}
		else
		{
			fail();
		}
	}
}

void FdChannel::flush()
{
	while (m_valid && !m_output.isEmpty())
	{
		const auto size = ::write(m_writeFd, m_output.constData(), size_t(m_output.size()));
		if (size > 0)
		{
			m_output.remove(0, int(size));
		}
		else if (size < 0 && errno == EINTR)
		{
			continue;
		}
		else if (size < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
		{
			break;
		}
		else
		{
			fail();
		}
	}
	if (m_writer)
	{
		m_writer->setEnabled(m_valid && !m_output.isEmpty());
	}
}

void FdChannel::fail()
{
	if (!m_valid)
	{
		return;
	}
	m_valid = false;
	m_reader->setEnabled(false);
	m_writer->setEnabled(false);
	Q_EMIT disconnected();
}
