/*
 * GreeterVncServer.h - isolated greeter capture backend supervision
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

class GreeterVncServer
{
public:
	enum class Backend { X11, Headless };

	static QList<Backend> backendsForSession(const QString& sessionType);
	static QString backendName(Backend backend);
	static bool runForSession(const QString& program, const QString& sessionType, quint16 port,
							  const QByteArray& password, int startupTimeout = 10000);

	// Blocks until the backend exits. Returns whether it completed a
	// password-authenticated RFB handshake, including framebuffer init.
	static bool run(const QString& program, Backend backend, quint16 port,
					const QByteArray& password, int startupTimeout = 10000);
};
