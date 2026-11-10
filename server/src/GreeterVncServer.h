/*
 * GreeterVncServer.h - isolated greeter capture backend supervision
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QProcessEnvironment>
#include <functional>

class GreeterVncServer
{
public:
	enum class Backend { X11, KWin, Headless };
	using EnvironmentProvider = std::function<QProcessEnvironment()>;

	static QList<Backend> backendsForSession(const QString& sessionType);
	static QString backendName(Backend backend);
	static bool runForSession(const QString& program, const QString& sessionType, quint16 port,
							  const QByteArray& password, int startupTimeout = 10000,
							  const EnvironmentProvider& environmentProvider = {});

	// Blocks until the backend exits. Returns whether it completed a
	// password-authenticated RFB handshake, including framebuffer init.
	static bool run(const QString& program, Backend backend, quint16 port,
					const QByteArray& password, int startupTimeout = 10000,
					const QProcessEnvironment& environment = QProcessEnvironment::systemEnvironment(),
					const std::function<bool()>& restartRequested = {});
};
