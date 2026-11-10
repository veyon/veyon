/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "LoginSession.h"

#include <QDBusArgument>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusReply>

bool LoginSession::isGraphical() const
{
	return type == QLatin1String("x11") || type == QLatin1String("wayland");
}

bool LoginSession::isOpen() const
{
	return state == QLatin1String("active") || state == QLatin1String("online");
}

bool LoginSession::isLightDmGreeter() const
{
	// Inspect the running session, not installed packages or the user's next
	// desktop choice. Some distributions classify greeters as "user".
	// LightDM greeter PAM stacks may omit XDG_SESSION_TYPE. Registration still
	// requires the installed wrapper directly parented by the root LightDM
	// session leader; this is never sufficient by itself to authorize a login.
	const bool greeterType = isGraphical() || type.isEmpty() || type == QLatin1String("unspecified");
	return service == QLatin1String("lightdm-greeter") && greeterType && isOpen() && !seat.isEmpty();
}

bool LoginSession::isUser() const
{
	return sessionClass == QLatin1String("user") && service != QLatin1String("lightdm-greeter") &&
		isGraphical() && isOpen();
}

LoginSession LoginSession::read(const QDBusConnection& bus, const QString& path)
{
	LoginSession result;
	if (!path.startsWith(QLatin1String("/org/freedesktop/login1/session/")))
	{
		return result;
	}
	QDBusInterface properties(QStringLiteral("org.freedesktop.login1"), path,
		QStringLiteral("org.freedesktop.DBus.Properties"), bus);
	const QDBusReply<QVariantMap> reply = properties.call(QStringLiteral("GetAll"),
		QStringLiteral("org.freedesktop.login1.Session"));
	if (!reply.isValid() || reply.value().isEmpty())
	{
		return result;
	}
	const auto values = reply.value();
	result.path = path;
	result.id = values.value(QStringLiteral("Id")).toString();
	result.name = values.value(QStringLiteral("Name")).toString();
	result.service = values.value(QStringLiteral("Service")).toString();
	result.type = values.value(QStringLiteral("Type")).toString();
	result.display = values.value(QStringLiteral("Display")).toString();
	result.state = values.value(QStringLiteral("State")).toString();
	result.sessionClass = values.value(QStringLiteral("Class")).toString();
	result.leader = values.value(QStringLiteral("Leader")).toUInt();
	const auto user = qvariant_cast<QDBusArgument>(values.value(QStringLiteral("User")));
	const auto seat = qvariant_cast<QDBusArgument>(values.value(QStringLiteral("Seat")));
	if (user.currentType() != QDBusArgument::StructureType || seat.currentType() != QDBusArgument::StructureType)
	{
		return {};
	}
	QDBusObjectPath userPath;
	user.beginStructure();
	user >> result.uid >> userPath;
	user.endStructure();
	QDBusObjectPath seatPath;
	seat.beginStructure();
	seat >> result.seat >> seatPath;
	seat.endStructure();
	return result;
}

QVector<LoginSession> LoginSession::list(const QDBusConnection& bus, bool* ok)
{
	QVector<LoginSession> result;
	QDBusInterface manager(QStringLiteral("org.freedesktop.login1"), QStringLiteral("/org/freedesktop/login1"),
		QStringLiteral("org.freedesktop.login1.Manager"), bus);
	const auto reply = manager.call(QStringLiteral("ListSessions"));
	*ok = reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() == 1;
	if (!*ok)
	{
		return result;
	}
	const auto sessions = qvariant_cast<QDBusArgument>(reply.arguments().first());
	sessions.beginArray();
	while (!sessions.atEnd())
	{
		QString id, name, seat;
		uint uid;
		QDBusObjectPath path;
		sessions.beginStructure();
		sessions >> id >> uid >> name >> seat >> path;
		sessions.endStructure();
		const auto session = read(bus, path.path());
		if (session.path.isEmpty())
		{
			// Fail closed for this snapshot; the caller may retry after a
			// session which disappeared during enumeration has settled.
			*ok = false;
			return {};
		}
		result.append(session);
	}
	sessions.endArray();
	return result;
}

QString LoginSession::forPid(const QDBusConnection& bus, uint pid)
{
	QDBusInterface manager(QStringLiteral("org.freedesktop.login1"), QStringLiteral("/org/freedesktop/login1"),
		QStringLiteral("org.freedesktop.login1.Manager"), bus);
	const QDBusReply<QDBusObjectPath> reply = manager.call(QStringLiteral("GetSessionByPID"), pid);
	return reply.isValid() ? reply.value().path() : QString();
}
