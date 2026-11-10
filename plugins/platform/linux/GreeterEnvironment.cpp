/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "GreeterEnvironment.h"
#include "SessionFileAccess.h"
#include "login-helper/LoginSession.h"

#include <QDir>
#include <QRegularExpression>

QProcessEnvironment GreeterEnvironment::prepare(const LoginSession& session, QProcessEnvironment environment)
{
	if (session.sessionClass != QLatin1String("greeter") && !session.isLightDmGreeter())
	{
		return environment;
	}

	// A Wayland greeter may also have DISPLAY for Xwayland. Capturing that
	// display would not show the compositor's login screen.
	if (session.type == QLatin1String("wayland"))
	{
		return environment;
	}
	const bool unspecified = session.type.isEmpty() || session.type == QLatin1String("unspecified");
	const bool x11 = session.type == QLatin1String("x11") ||
		(unspecified &&
		 (environment.value(QStringLiteral("XDG_SESSION_TYPE")) == QLatin1String("x11") ||
		  (environment.value(QStringLiteral("WAYLAND_DISPLAY")).isEmpty() &&
		   (!session.display.isEmpty() || !environment.value(QStringLiteral("DISPLAY")).isEmpty()))));
	if (!x11)
	{
		return environment;
	}
	if (!session.display.isEmpty())
	{
		// The Display property belongs to this logind session; descendants can
		// carry DISPLAY from nested displays or stale session setup scripts.
		environment.insert(QStringLiteral("DISPLAY"), session.display);
	}
	environment.remove(QStringLiteral("WAYLAND_DISPLAY"));
	environment.insert(QStringLiteral("XDG_SESSION_TYPE"), QStringLiteral("x11"));
	environment.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("xcb"));

	const auto authority = environment.value(QStringLiteral("XAUTHORITY"));
	if (!SessionFileAccess::isXAuthoritySafe(authority, session.uid, true))
	{
		environment.remove(QStringLiteral("XAUTHORITY"));
		const auto home = environment.value(QStringLiteral("HOME"));
		if (QDir::isAbsolutePath(home))
		{
			const auto defaultAuthority = QDir(home).filePath(QStringLiteral(".Xauthority"));
			if (SessionFileAccess::isXAuthoritySafe(defaultAuthority, session.uid, true))
			{
				// Make the checked default explicit so a capture retry never
				// depends on an unchecked implicit Xlib authority path.
				environment.insert(QStringLiteral("XAUTHORITY"), defaultAuthority);
			}
		}
		if (!environment.contains(QStringLiteral("XAUTHORITY")) && session.service == QLatin1String("lightdm-greeter"))
		{
			// LightDM keeps the local X server's cookie here. Only consider the
			// exact display of this session, and retain all ownership checks.
			static const QRegularExpression localDisplay(QStringLiteral(R"(^(:[0-9]+)(?:\.[0-9]+)?$)"));
			const auto match = localDisplay.match(environment.value(QStringLiteral("DISPLAY")));
			if (match.hasMatch())
			{
				const auto serverAuthority = QStringLiteral("/run/lightdm/root/") + match.captured(1);
				if (SessionFileAccess::isXAuthoritySafe(serverAuthority, 0, true))
				{
					environment.insert(QStringLiteral("XAUTHORITY"), serverAuthority);
				}
			}
		}
	}
	return environment;
}
