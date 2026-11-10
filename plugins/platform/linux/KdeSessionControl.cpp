/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "KdeSessionControl.h"

#include <QDBusError>
#include "LinuxDesktopIntegration.h"

namespace
{
bool isKdeSession(const QProcessEnvironment& environment)
{
	for (const auto* variable : {"XDG_CURRENT_DESKTOP", "XDG_SESSION_DESKTOP", "DESKTOP_SESSION"})
	{
		const auto desktop = environment.value(QString::fromLatin1(variable)).trimmed();
		if (desktop.isEmpty()) { continue; }
		const auto names = desktop.toLower().split(QLatin1Char(':'), Qt::SkipEmptyParts);
		for (const auto& name : names)
		{
			if (name == QLatin1String("kde") || name == QLatin1String("plasma") ||
				name == QLatin1String("plasmax11") || name == QLatin1String("plasmawayland") ||
				name == QLatin1String("plasma-x11") || name == QLatin1String("plasma-wayland")) { return true; }
		}
		return false;
	}
	const auto fullSession = environment.value(QStringLiteral("KDE_FULL_SESSION"));
	return fullSession == QLatin1String("1") || fullSession.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
}
}

bool KdeSessionControl::isUnavailable(const QDBusMessage& reply)
{
	const auto error = QDBusError(reply).type();
	return reply.type() == QDBusMessage::ErrorMessage &&
		(error == QDBusError::ServiceUnknown || error == QDBusError::UnknownObject ||
		 error == QDBusError::UnknownInterface || error == QDBusError::UnknownMethod ||
		 error == QDBusError::NotSupported);
}

QDBusMessage KdeSessionControl::request(Operation operation, const QDBusConnection& sessionBus,
	const QProcessEnvironment& environment)
{
	if (!isKdeSession(environment))
	{
		return QDBusMessage::createError(QDBusError::ServiceUnknown, QStringLiteral("The active desktop is not KDE"));
	}
	QString method;
	int legacyType;
	switch (operation)
	{
	case Operation::Logout:
		method = QStringLiteral("logout");
		legacyType = int(LinuxDesktopIntegration::KDE::ShutdownTypeLogout);
		break;
	case Operation::Reboot:
		method = QStringLiteral("logoutAndReboot");
		legacyType = int(LinuxDesktopIntegration::KDE::ShutdownTypeReboot);
		break;
	case Operation::PowerOff:
		method = QStringLiteral("logoutAndShutdown");
		legacyType = int(LinuxDesktopIntegration::KDE::ShutdownTypeHalt);
		break;
	default:
		return QDBusMessage::createError(QDBusError::InvalidArgs, QStringLiteral("Unknown KDE session operation"));
	}

	// Plasma 6 exposes these no-argument methods on a separate, D-Bus activated
	// service. KSMServer.logout(int,int,int) is retained only for older Plasma.
	auto call = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Shutdown"), QStringLiteral("/Shutdown"),
		QStringLiteral("org.kde.Shutdown"), method);
	const auto reply = sessionBus.call(call, QDBus::Block, 5000);
	if (!isUnavailable(reply)) { return reply; }

	// Do not retry after AccessDenied or an ambiguous timeout: the modern
	// service may already be handling the request or may have refused it.
	call = QDBusMessage::createMethodCall(QStringLiteral("org.kde.ksmserver"), QStringLiteral("/KSMServer"),
		QStringLiteral("org.kde.KSMServerInterface"), QStringLiteral("logout"));
	call.setArguments({int(LinuxDesktopIntegration::KDE::ShutdownConfirmNo), legacyType,
		int(LinuxDesktopIntegration::KDE::ShutdownModeForceNow)});
	return sessionBus.call(call, QDBus::Block, 5000);
}
