/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "LightDmSession.h"
#include "ProcessIdentity.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>

namespace
{
QByteArray lightDmProcessData(uint pid, const QString& name)
{
	QFile file(QStringLiteral("/proc/%1/%2").arg(pid).arg(name));
	return file.open(QIODevice::ReadOnly) ? file.read(128 * 1024) : QByteArray();
}

QStringList lightDmConfigurationFiles(uint pid)
{
	const auto environment = lightDmProcessData(pid, QStringLiteral("environ")).split('\0');
	const auto paths = [&environment](const QByteArray& name, const QString& defaults) {
		QByteArray prefix = name;
		prefix.append('=');
		for (const auto& value : environment)
		{
			if (value.startsWith(prefix))
			{
				const auto configured = QString::fromLocal8Bit(value.mid(name.size() + 1));
				if (!configured.isEmpty()) { return configured.split(QLatin1Char(':'), Qt::SkipEmptyParts); }
			}
		}
		return defaults.split(QLatin1Char(':'), Qt::SkipEmptyParts);
	};
	QStringList files;
	const auto appendDirectory = [&files](const QString& path) {
		const QDir directory(path);
		for (const auto& name : directory.entryList({QStringLiteral("*.conf")}, QDir::Files, QDir::Name))
		{
			files.append(directory.filePath(name));
		}
	};
	for (const auto& directories : {paths("XDG_DATA_DIRS", QStringLiteral("/usr/local/share:/usr/share")),
		paths("XDG_CONFIG_DIRS", QStringLiteral("/etc/xdg"))})
	{
		for (auto it = directories.crbegin(); it != directories.crend(); ++it)
		{
			appendDirectory(QDir(*it).filePath(QStringLiteral("lightdm/lightdm.conf.d")));
		}
	}
	QString config;
	const auto arguments = lightDmProcessData(pid, QStringLiteral("cmdline")).split('\0');
	for (int i = 1; i < arguments.size(); ++i)
	{
		if ((arguments[i] == "--config" || arguments[i] == "-c") && i + 1 < arguments.size())
		{
			config = QString::fromLocal8Bit(arguments[++i]);
		}
		else if (arguments[i].startsWith("--config=")) { config = QString::fromLocal8Bit(arguments[i].mid(9)); }
		else if (arguments[i].startsWith("-c") && arguments[i].size() > 2) { config = QString::fromLocal8Bit(arguments[i].mid(2)); }
	}
	if (config.isEmpty())
	{
		appendDirectory(QStringLiteral("/etc/lightdm/lightdm.conf.d"));
		config = QStringLiteral("/etc/lightdm/lightdm.conf");
	}
	else if (QDir::isRelativePath(config))
	{
		config = QDir(QFileInfo(QStringLiteral("/proc/%1/cwd").arg(pid)).canonicalFilePath()).filePath(config);
	}
	files.append(config);
	return files;
}
}

bool LightDmSession::isAvailable(const QString& session, uint greeterLeader)
{
	if (session.isEmpty()) { return true; }
	const auto child = ProcessIdentity::read(greeterLeader);
	const auto daemon = ProcessIdentity::read(child.parentPid);
	if (!child.isLightDmSessionChildOf(daemon)) { return false; }
	const auto lightdmPid = daemon.pid;
	const auto arguments = lightDmProcessData(lightdmPid, QStringLiteral("cmdline")).split('\0');
	if (arguments.isEmpty() || arguments.first().isEmpty() || arguments.contains("--session-child")) { return false; }
	const auto executable = QFileInfo(QStringLiteral("/proc/%1/exe").arg(lightdmPid)).canonicalFilePath();
	QDir prefix(QFileInfo(executable).absolutePath());
	prefix.cdUp();
	const auto data = executable.isEmpty() ? QStringLiteral("/usr/share") : prefix.filePath(QStringLiteral("share"));
	const bool available = isAvailable(session, lightDmConfigurationFiles(lightdmPid),
		{QDir(data).filePath(QStringLiteral("lightdm/sessions")), QDir(data).filePath(QStringLiteral("xsessions")),
		 QDir(data).filePath(QStringLiteral("wayland-sessions"))});
	return available && child.sameProcess(ProcessIdentity::read(child.pid)) &&
		daemon.sameProcess(ProcessIdentity::read(daemon.pid));
}

bool LightDmSession::isAvailable(const QString& session, const QStringList& configurationFiles, const QStringList& defaultDirectories)
{
	if (session.isEmpty()) { return true; }
	static const QRegularExpression identifier(QStringLiteral("^[A-Za-z0-9_.+-]+$"));
	if (session.size() > 256 || !identifier.match(session).hasMatch()) { return false; }
	auto directories = defaultDirectories;
	for (const auto& path : configurationFiles)
	{
		QSettings config(path, QSettings::IniFormat);
		const auto value = config.value(QStringLiteral("LightDM/sessions-directory"));
		if (value.isValid()) { directories = value.toString().split(QLatin1Char(':'), Qt::SkipEmptyParts); }
	}
	for (const auto& directory : directories)
	{
		if (!QDir::isAbsolutePath(directory)) { continue; }
		const auto path = QDir(directory).filePath(session + QStringLiteral(".desktop"));
		if (!QFileInfo(path).isFile()) { continue; }
		QSettings definition(path, QSettings::IniFormat);
		if (definition.status() == QSettings::NoError &&
			!definition.value(QStringLiteral("Desktop Entry/Exec")).toString().trimmed().isEmpty()) { return true; }
	}
	return false;
}
