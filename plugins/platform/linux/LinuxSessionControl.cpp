// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-FileCopyrightText: 2026 Ilya Mukhamadeev <nicourced@altlinux.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDBusConnection>
#include <QDBusError>

#include "LinuxDBusInterfaces.h"
#include "LinuxDesktopIntegration.h"
#include "LinuxSessionControl.h"
#include "VeyonCore.h"

#include <algorithm>


LinuxSessionControl::LinuxSessionControl(const Backend& backend, const QProcessEnvironment& environment) :
	m_backend(backend),
	m_environment(environment)
{
}



LinuxSessionControl::Result LinuxSessionControl::requestReboot() const
{
	return requestPowerAction(Action::Reboot, QStringLiteral("Reboot"), QStringLiteral("Restart"));
}



LinuxSessionControl::Result LinuxSessionControl::requestPowerOff() const
{
	return requestPowerAction(Action::PowerOff, QStringLiteral("PowerOff"), QStringLiteral("Stop"));
}



LinuxSessionControl::Result LinuxSessionControl::requestLogout() const
{
	m_backend.prepareSessionBusAccess();

	const auto result = requestFromDesktop(Action::Logout);
	switch (result)
	{
	case Result::Accepted:
		return result;
	case Result::Unavailable:
		m_backend.runLogoutCommands();
		break;
	case Result::Failed:
		// a desktop session manager exists but refused or is busy - do not try other desktops'
		// commands, but end the session nonetheless
		vWarning() << "desktop session manager did not log out - terminating session";
		break;
	}

	terminateSession();

	return result;
}



QDBusMessage LinuxSessionControl::callSessionBus(const QDBusMessage& methodCall, int timeout)
{
	return QDBusConnection::sessionBus().call(methodCall, QDBus::Block, timeout);
}



bool LinuxSessionControl::isKdeEnvironment(const QProcessEnvironment& environment)
{
	// a positive hint only: any of the variables naming KDE suffices, as XDG_CURRENT_DESKTOP may have
	// been rewritten to X-Generic (#817); XDG_CURRENT_DESKTOP may contain a colon-separated list
	static const QStringList DesktopVariables{
		QStringLiteral("XDG_CURRENT_DESKTOP"),
		QStringLiteral("XDG_SESSION_DESKTOP"),
		QStringLiteral("DESKTOP_SESSION"),
	};
	static const QStringList KdeDesktopNames{
		QStringLiteral("kde"),
		QStringLiteral("plasma"),
		QStringLiteral("plasma5"),
		QStringLiteral("plasma6"),
		QStringLiteral("plasmax11"),
		QStringLiteral("plasmawayland"),
		QStringLiteral("plasma-x11"),
		QStringLiteral("plasma-wayland"),
	};

	for (const auto& variable : DesktopVariables)
	{
		// DESKTOP_SESSION may be the path of the session's desktop file
		const auto desktop = environment.value(variable).trimmed().toLower().section(QLatin1Char('/'), -1);
		const auto names = desktop.split(QLatin1Char(':'), Qt::SkipEmptyParts);
		const auto isKdeName = [](const QString& name) {
			return KdeDesktopNames.contains(name.section(QLatin1Char('.'), 0, 0));
		};
		if (std::any_of(names.cbegin(), names.cend(), isKdeName))
		{
			return true;
		}
	}

	const auto fullSession = environment.value(QStringLiteral("KDE_FULL_SESSION"));
	return fullSession == QLatin1String("1") ||
		fullSession.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
}



bool LinuxSessionControl::isUnavailable(const QDBusMessage& reply)
{
	if (reply.type() != QDBusMessage::ErrorMessage)
	{
		return false;
	}

	switch (QDBusError(reply).type())
	{
	case QDBusError::ServiceUnknown:
	case QDBusError::UnknownObject:
	case QDBusError::UnknownInterface:
	case QDBusError::UnknownMethod:
	// no session bus at all (e.g. no session user)
	case QDBusError::Disconnected:
		return true;
	default:
		break;
	}

	// activation of a D-Bus activated service failed (dbus-daemon or systemd)
	static const QStringList ActivationErrors{
		QStringLiteral("org.freedesktop.DBus.Error.NameHasNoOwner"),
		QStringLiteral("org.freedesktop.systemd1.NoSuchUnit"),
	};
	static const auto SpawnErrorPrefix = QStringLiteral("org.freedesktop.DBus.Error.Spawn.");

	const auto errorName = reply.errorName();
	return ActivationErrors.contains(errorName) || errorName.startsWith(SpawnErrorPrefix);
}



LinuxSessionControl::Result LinuxSessionControl::requestPowerAction(Action action, const QString& logindMethod,
																	const QString& consoleKitMethod) const
{
	// interactive = false: never wait for a polkit authentication dialog
	const auto logindCall = QDBusMessage::createMethodCall(LogindManagerInterface::ServiceName,
														   LogindManagerInterface::ObjectPath,
														   LogindManagerInterface::InterfaceName, logindMethod)
		<< false;
	const auto logindReply = m_backend.systemBusCall(logindCall);
	if (isAccepted(logindReply))
	{
		return Result::Accepted;
	}
	if (isUnavailable(logindReply) == false)
	{
		vWarning() << "logind" << logindMethod << "request failed:"
				   << logindReply.errorName() << logindReply.errorMessage();
	}

	const auto consoleKitCall = QDBusMessage::createMethodCall(ConsoleKitManagerInterface::ServiceName,
															   ConsoleKitManagerInterface::ObjectPath,
															   ConsoleKitManagerInterface::InterfaceName, consoleKitMethod);
	const auto consoleKitReply = m_backend.systemBusCall(consoleKitCall);
	if (isAccepted(consoleKitReply))
	{
		return Result::Accepted;
	}
	if (isUnavailable(consoleKitReply) == false)
	{
		vWarning() << "ConsoleKit" << consoleKitMethod << "request failed:"
				   << consoleKitReply.errorName() << consoleKitReply.errorMessage();
	}

	m_backend.prepareSessionBusAccess();

	const auto result = requestFromDesktop(action);
	if (result == Result::Unavailable)
	{
		vCritical() << "no login or desktop session manager accepted the" << logindMethod << "request";
	}

	return result;
}



LinuxSessionControl::Result LinuxSessionControl::requestFromDesktop(Action action) const
{
	for (const auto& methodCall : desktopMethodCalls(action))
	{
		const auto reply = m_backend.sessionBusCall(methodCall);
		if (isAccepted(reply))
		{
			vDebug() << methodCall.service() << methodCall.member() << "accepted";
			return Result::Accepted;
		}

		if (isUnavailable(reply) == false)
		{
			vCritical() << methodCall.service() << methodCall.member() << "request failed:"
						<< reply.errorName() << reply.errorMessage();
			return Result::Failed;
		}

		vDebug() << methodCall.service() << methodCall.member() << "unavailable:" << reply.errorName();
	}

	return Result::Unavailable;
}



void LinuxSessionControl::terminateSession() const
{
	const auto terminateCall = QDBusMessage::createMethodCall(LogindManagerInterface::ServiceName,
															  LogindManagerInterface::ObjectPath,
															  LogindManagerInterface::InterfaceName,
															  LogindManagerInterface::TerminateSessionMethod)
		<< m_environment.value(QStringLiteral("XDG_SESSION_ID"));
	const auto terminateReply = m_backend.systemBusCall(terminateCall);
	if (isAccepted(terminateReply) == false && isUnavailable(terminateReply) == false)
	{
		vWarning() << "logind TerminateSession request failed:"
				   << terminateReply.errorName() << terminateReply.errorMessage();
	}

	// close the session via ConsoleKit as a last resort
	const auto closeCall = QDBusMessage::createMethodCall(ConsoleKitManagerInterface::ServiceName,
														  ConsoleKitManagerInterface::ObjectPath,
														  ConsoleKitManagerInterface::InterfaceName,
														  ConsoleKitManagerInterface::CloseSessionMethod)
		<< m_environment.value(QStringLiteral("XDG_SESSION_COOKIE"));
	const auto closeReply = m_backend.systemBusCall(closeCall);
	if (isAccepted(closeReply) == false && isUnavailable(closeReply) == false)
	{
		vWarning() << "ConsoleKit CloseSession request failed:" << closeReply.errorName() << closeReply.errorMessage();
	}
}



QList<QDBusMessage> LinuxSessionControl::desktopMethodCalls(Action action) const
{
	using KDE = LinuxDesktopIntegration::KDE;

	const auto gnomeCall = [](const QString& method) {
		return QDBusMessage::createMethodCall(GnomeSessionManagerInterface::ServiceName,
											  GnomeSessionManagerInterface::ObjectPath,
											  GnomeSessionManagerInterface::InterfaceName, method);
	};
	const auto mateCall = [](const QString& method) {
		return QDBusMessage::createMethodCall(MateSessionManagerInterface::ServiceName,
											  MateSessionManagerInterface::ObjectPath,
											  MateSessionManagerInterface::InterfaceName, method);
	};
	const auto powerManagementCall = [](const QString& method) {
		return QDBusMessage::createMethodCall(PowerManagementInterface::ServiceName,
											  PowerManagementInterface::ObjectPath,
											  PowerManagementInterface::InterfaceName, method);
	};

	QList<QDBusMessage> methodCalls;

	switch (action)
	{
	case Action::Logout:
		appendKdeMethodCalls(methodCalls, KdeShutdownInterface::LogoutMethod, KDE::ShutdownTypeLogout);
		methodCalls.append(gnomeCall(QStringLiteral("Logout"))
						   << static_cast<uint>(LinuxDesktopIntegration::Gnome::GSM_MANAGER_LOGOUT_MODE_FORCE));
		methodCalls.append(mateCall(QStringLiteral("Logout"))
						   << static_cast<uint>(LinuxDesktopIntegration::Mate::GSM_LOGOUT_MODE_FORCE));
		break;
	case Action::Reboot:
		appendKdeMethodCalls(methodCalls, KdeShutdownInterface::RebootMethod, KDE::ShutdownTypeReboot);
		methodCalls.append(gnomeCall(QStringLiteral("RequestReboot")));
		methodCalls.append(mateCall(QStringLiteral("RequestReboot")));
		methodCalls.append(powerManagementCall(QStringLiteral("Reboot")));
		break;
	case Action::PowerOff:
		appendKdeMethodCalls(methodCalls, KdeShutdownInterface::PowerOffMethod, KDE::ShutdownTypeHalt);
		methodCalls.append(gnomeCall(QStringLiteral("RequestShutdown")));
		methodCalls.append(mateCall(QStringLiteral("RequestShutdown")));
		methodCalls.append(powerManagementCall(QStringLiteral("Shutdown")));
		break;
	}

	return methodCalls;
}



void LinuxSessionControl::appendKdeMethodCalls(QList<QDBusMessage>& methodCalls, const QString& shutdownMethod,
											   int ksmServerShutdownType) const
{
	if (isKdeSession() == false)
	{
		return;
	}

	using KDE = LinuxDesktopIntegration::KDE;

	methodCalls.append(QDBusMessage::createMethodCall(KdeShutdownInterface::ServiceName,
													  KdeShutdownInterface::ObjectPath,
													  KdeShutdownInterface::InterfaceName, shutdownMethod));
	methodCalls.append(QDBusMessage::createMethodCall(KsmServerInterface::ServiceName,
													  KsmServerInterface::ObjectPath,
													  KsmServerInterface::InterfaceName, KsmServerInterface::LogoutMethod)
					   << static_cast<int>(KDE::ShutdownConfirmNo)
					   << ksmServerShutdownType
					   << static_cast<int>(KDE::ShutdownModeForceNow));
}



bool LinuxSessionControl::isKdeSession() const
{
	return isKdeEnvironment(m_environment) ||
		hasSessionBusOwner(KsmServerInterface::ServiceName) ||
		hasSessionBusOwner(KdeShutdownInterface::ServiceName);
}



bool LinuxSessionControl::hasSessionBusOwner(const QString& serviceName) const
{
	const auto reply = m_backend.sessionBusCall(QDBusMessage::createMethodCall(DBusDaemonInterface::ServiceName,
																			   DBusDaemonInterface::ObjectPath,
																			   DBusDaemonInterface::InterfaceName,
																			   DBusDaemonInterface::NameHasOwnerMethod)
												<< serviceName);

	return isAccepted(reply) && reply.arguments().value(0).toBool();
}



bool LinuxSessionControl::isAccepted(const QDBusMessage& reply)
{
	return reply.type() == QDBusMessage::ReplyMessage;
}
