// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>

#include "LinuxCoreFunctions.h"
#include "LinuxDBusInterfaces.h"
#include "LinuxDesktopIntegration.h"
#include "LinuxKeyboardInput.h"
#include "LinuxPlatformConfiguration.h"
#include "LinuxSessionFunctions.h"
#include "LinuxUserFunctions.h"
#include "VeyonConfiguration.h"

#define XK_MISCELLANY

#include <X11/Xlib.h>
#include <X11/keysymdef.h>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#include <vector>

#ifdef None
#undef None
#endif


QString LinuxUserFunctions::queryCurrentUserProperty(UserProperty property)
{
	switch (property)
	{
	case UserProperty::LoginName: return currentUserLoginName();
	case UserProperty::FullName: return currentUserFullName();
	case UserProperty::None: break;
	}
	return {};
}



QStringList LinuxUserFunctions::userGroups(bool queryDomainGroups)
{
	Q_UNUSED(queryDomainGroups)

	QStringList groupList;

	QProcess getentProcess;
	getentProcess.start(QStringLiteral("/usr/bin/getent"), {QStringLiteral("group")});
	getentProcess.waitForFinished();

	const auto groups = QString::fromUtf8(getentProcess.readAll()).split(QLatin1Char('\n'));

	groupList.reserve(groups.size());

	for (const auto& group : groups)
	{
		groupList += group.split(QLatin1Char(':')).first();
	}

	const QStringList ignoredGroups({
		QStringLiteral("daemon"),
		QStringLiteral("bin"),
		QStringLiteral("tty"),
		QStringLiteral("disk"),
		QStringLiteral("lp"),
		QStringLiteral("mail"),
		QStringLiteral("news"),
		QStringLiteral("uucp"),
		QStringLiteral("man"),
		QStringLiteral("proxy"),
		QStringLiteral("kmem"),
		QStringLiteral("dialout"),
		QStringLiteral("fax"),
		QStringLiteral("voice"),
		QStringLiteral("cdrom"),
		QStringLiteral("tape"),
		QStringLiteral("audio"),
		QStringLiteral("dip"),
		QStringLiteral("www-data"),
		QStringLiteral("backup"),
		QStringLiteral("list"),
		QStringLiteral("irc"),
		QStringLiteral("src"),
		QStringLiteral("gnats"),
		QStringLiteral("shadow"),
		QStringLiteral("utmp"),
		QStringLiteral("video"),
		QStringLiteral("sasl"),
		QStringLiteral("plugdev"),
		QStringLiteral("games"),
		QStringLiteral("nogroup"),
		QStringLiteral("libuuid"),
		QStringLiteral("syslog"),
		QStringLiteral("fuse"),
		QStringLiteral("lpadmin"),
		QStringLiteral("ssl-cert"),
		QStringLiteral("messagebus"),
		QStringLiteral("crontab"),
		QStringLiteral("mlocate"),
		QStringLiteral("avahi-autoipd"),
		QStringLiteral("netdev"),
		QStringLiteral("saned"),
		QStringLiteral("sambashare"),
		QStringLiteral("haldaemon"),
		QStringLiteral("polkituser"),
		QStringLiteral("mysql"),
		QStringLiteral("avahi"),
		QStringLiteral("klog"),
		QStringLiteral("floppy"),
		QStringLiteral("oprofile"),
		QStringLiteral("netdev"),
		QStringLiteral("dirmngr"),
		QStringLiteral("vboxusers"),
		QStringLiteral("bluetooth"),
		QStringLiteral("colord"),
		QStringLiteral("libvirtd"),
		QStringLiteral("nm-openvpn"),
		QStringLiteral("input"),
		QStringLiteral("kvm"),
		QStringLiteral("pulse"),
		QStringLiteral("pulse-access"),
		QStringLiteral("rtkit"),
		QStringLiteral("scanner"),
		QStringLiteral("sddm"),
		QStringLiteral("systemd-bus-proxy"),
		QStringLiteral("systemd-journal"),
		QStringLiteral("systemd-network"),
		QStringLiteral("systemd-resolve"),
		QStringLiteral("systemd-timesync"),
		QStringLiteral("utempter"),
		QStringLiteral("uuidd"),
	});

	for (const auto& ignoredGroup : ignoredGroups)
	{
		groupList.removeAll(ignoredGroup);
	}

	// remove all empty entries
	groupList.removeAll(QString());

	return groupList;
}



QStringList LinuxUserFunctions::groupsOfUser(const QString& username, bool queryDomainGroups)
{
	Q_UNUSED(queryDomainGroups)

	QStringList groupList;

	// query group memberships through NSS first - this also works with directory services
	// (SSSD, winbind, nslcd) which do not enumerate all groups (e.g. SSSD with enumerate=false)
	const auto user = username.toUtf8();
	const auto pw_entry = getpwnam(user.constData());
	if (pw_entry)
	{
		static constexpr int MaxGroupCount = 65536;
		const auto primaryGroupId = pw_entry->pw_gid;
		std::vector<gid_t> groupIds(64);
		auto groupCount = int(groupIds.size());
		while (getgrouplist(user.constData(), primaryGroupId, groupIds.data(), &groupCount) < 0)
		{
			// not all implementations report the required size
			groupCount = std::max(groupCount, int(groupIds.size()) * 2);
			if (groupCount > MaxGroupCount)
			{
				groupCount = 0;
				break;
			}
			groupIds.resize(groupCount);
		}

		for (int i = 0; i < groupCount; ++i)
		{
			const auto gr_entry = getgrgid(groupIds[i]);
			if (gr_entry && gr_entry->gr_name)
			{
				groupList.append(QString::fromUtf8(gr_entry->gr_name)); // clazy:exclude=reserve-candidates
			}
		}

		groupList.removeDuplicates();
		groupList.removeAll(QString());

		if (groupList.isEmpty() == false)
		{
			return groupList;
		}
	}

	// fall back to enumerating all groups
	QProcess getentProcess;
	getentProcess.start(QStringLiteral("/usr/bin/getent"), {QStringLiteral("group")});
	getentProcess.waitForFinished();

	const auto groups = QString::fromUtf8(getentProcess.readAll()).split(QLatin1Char('\n'));
	for (const auto& group : groups)
	{
		const auto groupComponents = group.split(QLatin1Char(':'));
		if (groupComponents.size() == 4 &&
			groupComponents.last().split(QLatin1Char(',')).contains(username))
		{
			groupList += groupComponents.first(); // clazy:exclude=reserve-candidates
		}
	}

	groupList.removeAll(QString());

	return groupList;
}



QString LinuxUserFunctions::userGroupSecurityIdentifier(const QString& groupName)
{
	const auto group = getgrnam(groupName.toUtf8().constData());
	if (group)
	{
		return QString::number(group->gr_gid);
	}

	return {};
}



bool LinuxUserFunctions::isAnyUserLoggedOn()
{
	const auto sessions = LinuxSessionFunctions::listSessions();
	for (const auto& session : sessions)
	{
		if (LinuxSessionFunctions::isOpen(session) &&
			LinuxSessionFunctions::isGraphical(session) &&
			LinuxSessionFunctions::getSessionClass(session) == LinuxSessionFunctions::Class::User)
		{
			return true;
		}
	}

	return false;
}



bool LinuxUserFunctions::prepareLogon(const QString& username, const Password& password)
{
	if (m_logonHelper.prepare(username, password))
	{
		LinuxCoreFunctions::restartDisplayManagers();
		return true;
	}

	return false;
}



bool LinuxUserFunctions::performLogon(const QString& username, const Password& password)
{
	LinuxKeyboardInput input;

	const LinuxPlatformConfiguration config(&VeyonCore::config());
	const auto textInputKeyPressInterval = config.userLoginTextInputKeyPressInterval();
	const auto controlKeyPressInterval = config.userLoginControlKeyPressInterval();

	auto sequence = config.userLoginKeySequence();

	if (sequence.isEmpty() == true)
	{
		sequence = QStringLiteral("%username%<Tab>%password%<Return>");
	}

	static const QRegularExpression keySequenceRX(QStringLiteral("(<[\\w\\d_]+>|%username%|%password%|[\\w\\d]+)"));
	auto matchIterator = keySequenceRX.globalMatch(sequence);
	if (matchIterator.hasNext() == false)
	{
		vCritical() << "invalid user login key sequence";
		return false;
	}

	QThread::msleep(config.userLoginInputStartDelay());

	while (matchIterator.hasNext())
	{
		const auto token = matchIterator.next().captured(0);
		if (token == QStringLiteral("%username%"))
		{
			input.sendString(username, textInputKeyPressInterval);
		}
		else if (token == QStringLiteral("%password%"))
		{
			input.sendString(QString::fromUtf8(password.toByteArray()), textInputKeyPressInterval);
		}
		else if (token.startsWith(QLatin1Char('<')) && token.endsWith(QLatin1Char('>')))
		{
			const auto keysymString = token.mid(1, token.length() - 2);
			const auto keysym = XStringToKeysym(keysymString.toLatin1().constData());
			if (keysym != NoSymbol)
			{
				input.pressAndReleaseKey(keysym);
				QThread::msleep(controlKeyPressInterval);
			}
			else
			{
				vCritical() << "unresolved keysym" << keysymString;
				return false;
			}
		}
		else if (token.isEmpty() == false)
		{
			input.sendString(token, textInputKeyPressInterval);
		}
	}

	return true;
}



void LinuxUserFunctions::logoff()
{
	VeyonCore::platform().coreFunctions().prepareSessionBusAccess();

	// logout via common session managers
	// logout via common session managers
	for (const auto& call : std::initializer_list<std::function<QDBusMessage()>>{
			 []() {
		return LinuxCoreFunctions::kdeSessionManager()
			->call(QStringLiteral("logout"),
				   LinuxDesktopIntegration::KDE::ShutdownConfirmNo,
				   LinuxDesktopIntegration::KDE::ShutdownTypeLogout,
				   LinuxDesktopIntegration::KDE::ShutdownModeForceNow);
	},
			 []() {
		return LinuxCoreFunctions::gnomeSessionManager()
			->call(QStringLiteral("Logout"),
				   LinuxDesktopIntegration::Gnome::GSM_MANAGER_LOGOUT_MODE_FORCE);
	},
			 []() {
		return LinuxCoreFunctions::mateSessionManager()
			->call(QStringLiteral("Logout"),
				   LinuxDesktopIntegration::Mate::GSM_LOGOUT_MODE_FORCE);
	}})
	{
		// call successful?
		if (call().type() == QDBusMessage::ReplyMessage)
		{
			return;
		}
	}

	// Xfce logout
	QProcess::startDetached(QStringLiteral("xfce4-session-logout --logout"), {});

	// LXDE logout
	QProcess::startDetached(QStringLiteral("kill -TERM %1").arg(QProcessEnvironment::systemEnvironment().value(QStringLiteral("_LXSESSION_PID")).toInt()), {});

	// terminate session via systemd
	LinuxCoreFunctions::systemdLoginManager()->asyncCall(QStringLiteral("TerminateSession"),
														 QProcessEnvironment::systemEnvironment().value(LinuxSessionFunctions::xdgSessionIdEnvVarName()));

	// close session via ConsoleKit as a last resort
	LinuxCoreFunctions::consoleKitManager()->asyncCall(QStringLiteral("CloseSession"),
													   QProcessEnvironment::systemEnvironment().value(QStringLiteral("XDG_SESSION_COOKIE")));
}



bool LinuxUserFunctions::authenticate(const QString& username, const Password& password)
{
	QDBusInterface service(VeyonServiceInterface::ServiceName,
						   VeyonServiceInterface::ObjectPath,
						   VeyonServiceInterface::InterfaceName,
						   QDBusConnection::systemBus());
	service.setTimeout(AuthHelperTimeout);

	const QDBusReply<bool> reply = service.call(VeyonServiceInterface::AuthenticateMethod,
												username, password.toByteArray());
	if (reply.isValid() == false)
	{
		vCritical() << "authentication request failed:" << reply.error().message();
		return false;
	}

	if (reply.value() == false)
	{
		vDebug() << "User authentication failed";
		return false;
	}

	vDebug() << "User authenticated successfully";
	return true;
}



LinuxUserFunctions::UserId LinuxUserFunctions::userIdFromName(const QString& username)
{
	const auto pw_entry = getpwnam(username.toUtf8().constData());

	if (pw_entry)
	{
		return pw_entry->pw_uid;
	}

	return InvalidUserId;
}



LinuxUserFunctions::GroupId LinuxUserFunctions::userGroupIdFromName(const QString& username)
{
	const auto pw_entry = getpwnam(username.toUtf8().constData());

	if (pw_entry)
	{
		return pw_entry->pw_gid;
	}

	return InvalidGroupId;
}



// sets HOME, USER, LOGNAME and SHELL from the password database and removes XDG base directory
// variables which point outside the user's home directory
bool LinuxUserFunctions::setUserEnvironment(QProcessEnvironment& environment, UserId userId)
{
	const auto suggestedBufferSize = sysconf(_SC_GETPW_R_SIZE_MAX);
	QByteArray buffer(suggestedBufferSize > 0 ? int(suggestedBufferSize) : 16384, Qt::Uninitialized);

	struct passwd pwEntry{};
	struct passwd* result = nullptr;
	int error = 0;
	while ((error = getpwuid_r(userId, &pwEntry, buffer.data(), size_t(buffer.size()), &result)) == ERANGE &&
		   buffer.size() < 1024 * 1024)
	{
		buffer.resize(buffer.size() * 2);
	}

	if (error != 0 || result == nullptr)
	{
		vWarning() << "could not look up user with ID" << userId << "-" << (error != 0 ? strerror(error) : "user not found");
		return false;
	}

	const auto homeDirectory = QDir::cleanPath(QString::fromUtf8(pwEntry.pw_dir));
	const auto userName = QString::fromUtf8(pwEntry.pw_name);

	environment.insert(QStringLiteral("HOME"), homeDirectory);
	environment.insert(QStringLiteral("USER"), userName);
	environment.insert(QStringLiteral("LOGNAME"), userName);
	if (pwEntry.pw_shell && *pwEntry.pw_shell)
	{
		environment.insert(QStringLiteral("SHELL"), QString::fromUtf8(pwEntry.pw_shell));
	}
	else
	{
		environment.remove(QStringLiteral("SHELL"));
	}

	// keep XDG base directories which are located in the home directory (also if it is reached via a
	// symlink) or in a directory owned by the user (e.g. /var/tmp/<user> with NFS homes), so that
	// values derived from a wrong HOME (e.g. "/.config") or pointing to other users are dropped
	const auto homeDirectoryPrefix = homeDirectory.endsWith(QLatin1Char('/')) ? homeDirectory
																			   : homeDirectory + QLatin1Char('/');
	const auto canonicalHomeDirectory = QFileInfo(homeDirectory).canonicalFilePath();
	const auto isUserDirectory = [&](const QString& path) {
		if (QDir::isAbsolutePath(path) == false)
		{
			return false;
		}

		if (QDir::cleanPath(path).startsWith(homeDirectoryPrefix))
		{
			return true;
		}

		const QFileInfo fileInfo(path);
		const auto canonicalPath = fileInfo.canonicalFilePath();
		if (canonicalPath.isEmpty())
		{
			return false;
		}

		return (canonicalHomeDirectory.isEmpty() == false &&
				(canonicalPath == canonicalHomeDirectory ||
				 canonicalPath.startsWith(canonicalHomeDirectory + QLatin1Char('/')))) ||
			   QFileInfo(canonicalPath).ownerId() == userId;
	};

	for (const auto& variable : {QStringLiteral("XDG_CONFIG_HOME"), QStringLiteral("XDG_DATA_HOME"),
								 QStringLiteral("XDG_CACHE_HOME"), QStringLiteral("XDG_STATE_HOME")})
	{
		if (environment.contains(variable) && isUserDirectory(environment.value(variable)) == false)
		{
			vDebug() << "discarding" << variable << "neither below home directory" << homeDirectory
					 << "nor owned by user";
			environment.remove(variable);
		}
	}

	return true;
}



QVariant LinuxUserFunctions::getUserProperty(const QString& userPath, const QString& property, bool logErrors)
{
	QDBusInterface loginManager(QStringLiteral("org.freedesktop.login1"),
								userPath,
								QStringLiteral("org.freedesktop.DBus.Properties"),
								QDBusConnection::systemBus());

	if (loginManager.connection().isConnected() == false)
	{
		vDebug() << "system bus not connected";
		return {};
	}

	const QDBusReply<QDBusVariant> reply = loginManager.call(QStringLiteral("Get"),
															 QStringLiteral("org.freedesktop.login1.User"),
															 property);

	if (reply.isValid() == false)
	{
		if (logErrors)
		{
			vCritical() << "Could not query property" << property
						<< "of user" << userPath
						<< "error:" << reply.error().message();
		}
		return {};
	}

	return reply.value().variant();
}



QString LinuxUserFunctions::currentUserLoginName() const
{
	QString username;

	if (VeyonCore::component() != VeyonCore::Component::CLI && m_systemBus.isConnected())
	{
		const auto sessionPath = LinuxSessionFunctions::currentSessionPath(true);
		if (sessionPath.isEmpty() == false)
		{
			const auto sessionUserPath = LinuxSessionFunctions::getSessionUser(sessionPath);
			if (sessionUserPath.isEmpty() == false)
			{
				username = getUserProperty(sessionUserPath, QStringLiteral("Name")).toString();
				if (username.isEmpty() == false)
				{
					return username;
				}
			}
		}
	}

	const auto envUser = qgetenv("USER");

	struct passwd* pw_entry = nullptr;
	if (envUser.isEmpty() == false)
	{
		pw_entry = getpwnam(envUser.constData());
	}

	if (pw_entry == nullptr)
	{
		pw_entry = getpwuid(getuid());
	}

	if (pw_entry)
	{
		const auto shell = QString::fromUtf8(pw_entry->pw_shell);

		// Skip not real users
		if (!(shell.endsWith(QLatin1String("/false")) ||
			  shell.endsWith(QLatin1String("/true")) ||
			  shell.endsWith(QLatin1String("/null")) ||
			  shell.endsWith(QLatin1String("/nologin"))))
		{
			username = QString::fromUtf8(pw_entry->pw_name);
		}
	}

	if (username.isEmpty())
	{
		return QString::fromUtf8(envUser);
	}

	return username;
}



QString LinuxUserFunctions::currentUserFullName() const
{
	const auto username = currentUserLoginName();
	const auto pw_entry = getpwnam(username.toUtf8().constData());

	if (pw_entry)
	{
		auto shell = QString::fromUtf8(pw_entry->pw_shell);

		// Skip not real users
		if (!(shell.endsWith(QLatin1String("/false")) ||
			  shell.endsWith(QLatin1String("/true")) ||
			  shell.endsWith(QLatin1String("/null")) ||
			  shell.endsWith(QLatin1String("/nologin"))))
		{
			return QString::fromUtf8(pw_entry->pw_gecos).split(QLatin1Char(',')).first();
		}
	}

	return {};
}
