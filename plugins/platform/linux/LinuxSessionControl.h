// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-FileCopyrightText: 2026 Ilya Mukhamadeev <nicourced@altlinux.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDBusMessage>
#include <QList>
#include <QProcessEnvironment>

#include <functional>

// Requests logout, reboot and power-off via D-Bus: reboot and power-off first from the system login
// manager (logind, then ConsoleKit), then from the session manager of the desktop, logout from the
// desktop session manager and, if none accepts it, by terminating the login session.
//
// A failed logind or ConsoleKit request (typically refused by polkit, e.g. for a power worker running as
// session user, or blocked by inhibitors or other sessions) falls through to the desktop, which applies
// its own authorisation and shutdown handling. An accepted request does not mean that the system is
// shut down: the desktop may still ask applications and end the session asynchronously.
//
// Desktop session managers are asked one after another and only as long as the previous one is not
// available (see isUnavailable()). Once a manager accepts the request, or refuses it or does not answer
// in time, no further desktop request is sent: a refused request must not be retried through another
// API, and after a timeout the manager may already be handling it. Logout then still terminates the
// session through logind (and ConsoleKit), as the teacher's request must not be ignored.
//
// In Plasma the no-argument methods of org.kde.Shutdown are used (as plasma-workspace's libkworkspace
// does). The service is D-Bus activated and therefore only called in a KDE session, so that installed
// Plasma packages do not take over another desktop. A KDE session is recognised by a running ksmserver
// (or org.kde.Shutdown) on the session bus or by the environment; the environment alone is not reliable
// as LinuxServiceCore reconstructs it and may rewrite XDG_CURRENT_DESKTOP=KDE to X-Generic (#817). The
// legacy KSMServer interface is used only when org.kde.Shutdown is unavailable (older Plasma versions).
//
// All D-Bus calls and the logout commands go through the functions passed to the constructor, so that
// the decisions can be tested without a bus. LinuxCoreFunctions::sessionControl() connects them to the
// real buses.
class LinuxSessionControl
{
public:
	enum class Action {
		Logout,
		Reboot,
		PowerOff,
	};

	enum class Result {
		Accepted,
		Failed,
		Unavailable,
	};

	using DBusCall = std::function<QDBusMessage(const QDBusMessage& methodCall)>;

	struct Backend
	{
		DBusCall systemBusCall;
		DBusCall sessionBusCall;
		// switch to the session user before the first session bus call
		std::function<void()> prepareSessionBusAccess;
		// desktop logout commands without D-Bus interface (Xfce, LXDE)
		std::function<void()> runLogoutCommands;
	};

	// in milliseconds - desktop session managers answer quickly or not at all, logind and ConsoleKit
	// keep the default timeout of Qt D-Bus
	static constexpr int SessionBusCallTimeout = 5000;

	LinuxSessionControl(const Backend& backend, const QProcessEnvironment& environment);

	Result requestReboot() const;
	Result requestPowerOff() const;
	// returns the result of the desktop session managers
	Result requestLogout() const;

	static QDBusMessage callSessionBus(const QDBusMessage& methodCall, int timeout = SessionBusCallTimeout);

	static bool isKdeEnvironment(const QProcessEnvironment& environment);
	static bool isUnavailable(const QDBusMessage& reply);

private:
	Result requestPowerAction(Action action, const QString& logindMethod, const QString& consoleKitMethod) const;
	Result requestFromDesktop(Action action) const;
	void terminateSession() const;
	QList<QDBusMessage> desktopMethodCalls(Action action) const;
	void appendKdeMethodCalls(QList<QDBusMessage>& methodCalls, const QString& shutdownMethod,
							  int ksmServerShutdownType) const;
	bool isKdeSession() const;
	bool hasSessionBusOwner(const QString& serviceName) const;

	static bool isAccepted(const QDBusMessage& reply);

	const Backend m_backend;
	const QProcessEnvironment m_environment;
};
