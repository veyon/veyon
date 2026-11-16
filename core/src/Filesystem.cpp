// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHostInfo>
#include <QRegularExpression>
#include <QStandardPaths>

#include "Filesystem.h"
#include "PlatformFilesystemFunctions.h"
#include "PlatformUserFunctions.h"
#include "VeyonConfiguration.h"


namespace {

bool hasUnsafeCharacters(const QString& value)
{
	for (const auto character : value)
	{
		if (character.unicode() < 0x20 || character.unicode() == 0x7f)
		{
			return true;
		}
	}

	return false;
}

bool isPathWithinBase(const QString& path, const QString& base)
{
#ifdef Q_OS_WIN
	return path.startsWith(base.endsWith(QLatin1Char('/')) ? base : base + QLatin1Char('/'), Qt::CaseInsensitive);
#else
	return path.startsWith(base.endsWith(QLatin1Char('/')) ? base : base + QLatin1Char('/'));
#endif
}

}



bool Filesystem::isSafePathComponent(const QString& name)
{
	// Windows device names (incl. COM0/LPT0, superscript digits and CONIN$/CONOUT$) are reserved
	// regardless of extension; trailing spaces before the extension are ignored by Windows
	static const QRegularExpression reservedName(
		QStringLiteral("^(CON|PRN|AUX|NUL|CONIN\\$|CONOUT\\$|COM[0-9\\x{00B9}\\x{00B2}\\x{00B3}]|LPT[0-9\\x{00B9}\\x{00B2}\\x{00B3}]) *(\\..*)?$"),
		QRegularExpression::CaseInsensitiveOption);

	return name.isEmpty() == false &&
		name.toUtf8().size() <= 255 &&
		hasUnsafeCharacters(name) == false &&
		name.contains(QLatin1Char('/')) == false &&
		name.contains(QLatin1Char('\\')) == false &&
		name.contains(QLatin1Char(':')) == false &&
		name != QStringLiteral(".") &&
		name != QStringLiteral("..") &&
		name.endsWith(QLatin1Char('.')) == false &&
		name.endsWith(QLatin1Char(' ')) == false &&
		reservedName.match(name).hasMatch() == false;
}



QString Filesystem::safeJoin(const QString& baseDir, const QString& relativePath)
{
	if (baseDir.isEmpty() || relativePath.isEmpty() || relativePath.size() > 4096 || hasUnsafeCharacters(relativePath))
	{
		vWarning() << "rejecting unsafe relative path";
		return {};
	}

	auto normalizedPath = relativePath;
	normalizedPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
	if (normalizedPath.startsWith(QLatin1Char('/')) ||
		normalizedPath.endsWith(QLatin1Char('/')) ||
		normalizedPath.contains(QLatin1Char(':')))
	{
		vWarning() << "rejecting unsafe relative path";
		return {};
	}

	const auto basePath = QDir(baseDir).absolutePath();
	if (QDir().mkpath(basePath) == false)
	{
		vWarning() << "failed to create base directory for safe path join";
		return {};
	}

	const auto canonicalBase = QDir(basePath).canonicalPath();
	if (canonicalBase.isEmpty())
	{
		vWarning() << "failed to canonicalize base directory for safe path join";
		return {};
	}

	QStringList components;
	for (const auto& component : normalizedPath.split(QLatin1Char('/'), Qt::SkipEmptyParts))
	{
		if (component == QStringLiteral("."))
		{
			continue;
		}

		if (isSafePathComponent(component) == false)
		{
			vWarning() << "rejecting unsafe relative path component";
			return {};
		}

		components.append(component);
	}

	if (components.isEmpty())
	{
		vWarning() << "rejecting unsafe relative path";
		return {};
	}

	const auto result = QDir::cleanPath(canonicalBase + QLatin1Char('/') + components.join(QLatin1Char('/')));
	if (isPathWithinBase(result, canonicalBase) == false)
	{
		vWarning() << "safe path join escaped base directory";
		return {};
	}

	auto existingPath = result;
	while (existingPath.isEmpty() == false && QFileInfo::exists(existingPath) == false)
	{
		const auto parentPath = QFileInfo(existingPath).dir().absolutePath();
		if (parentPath == existingPath)
		{
			vWarning() << "failed to find an existing parent directory for safe path join";
			return {};
		}

		existingPath = parentPath;
	}

	const auto canonicalExistingPath = QDir(existingPath).canonicalPath();
	if (canonicalExistingPath.isEmpty() || (canonicalExistingPath != canonicalBase && isPathWithinBase(canonicalExistingPath, canonicalBase) == false))
	{
		vWarning() << "safe path join resolved outside base directory";
		return {};
	}

	return result;
}



QString Filesystem::expandPath(QString path) const
{
	const auto p = QDir::toNativeSeparators(path.replace(QStringLiteral("%HOME%"), QDir::homePath()).replace(QStringLiteral("$HOME"), QDir::homePath()).replace(QStringLiteral("%HOSTNAME%"), QHostInfo::localHostName()).replace(QStringLiteral("$HOSTNAME"), QHostInfo::localHostName()).replace(QStringLiteral("%PROFILE%"), QDir::homePath()).replace(QStringLiteral("$PROFILE"), QDir::homePath()).replace(QStringLiteral("%USER%"), VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName)).replace(QStringLiteral("%DESKTOP%"), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).replace(QStringLiteral("%DOCUMENTS%"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).replace(QStringLiteral("%DOWNLOADS%"), QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).replace(QStringLiteral("%PICTURES%"), QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)).replace(QStringLiteral("%VIDEOS%"), QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)).replace(QStringLiteral("%APPDATA%"), VeyonCore::platform().filesystemFunctions().personalAppDataPath()).replace(QStringLiteral("$APPDATA"), VeyonCore::platform().filesystemFunctions().personalAppDataPath()).replace(QStringLiteral("%GLOBALAPPDATA%"), VeyonCore::platform().filesystemFunctions().globalAppDataPath()).replace(QStringLiteral("$GLOBALAPPDATA"), VeyonCore::platform().filesystemFunctions().globalAppDataPath()).replace(QStringLiteral("%TMP%"), QDir::tempPath()).replace(QStringLiteral("$TMP"), QDir::tempPath()).replace(QStringLiteral("%TEMP%"), QDir::tempPath()).replace(QStringLiteral("$TEMP"), QDir::tempPath()));

	// remove duplicate directory separators - however skip the first two chars
	// as they might specify an UNC path on Windows
	if (p.length() > 3)
	{
		return p.left(2) + p.mid(2).replace(QString(QStringLiteral("%1%1")).arg(QDir::separator()), QDir::separator());
	}

	return p;
}



QString Filesystem::shrinkPath(QString path) const
{
	path = QDir::toNativeSeparators(path);

	const auto tempPath = QDir::toNativeSeparators(QDir::tempPath());
	const auto personalAppDataPath = VeyonCore::platform().filesystemFunctions().personalAppDataPath();
	const auto globalAppDataPath = VeyonCore::platform().filesystemFunctions().globalAppDataPath();
	const auto desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
	const auto documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	const auto downloadsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
	const auto picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
	const auto videosPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
	const auto homePath = QDir::toNativeSeparators(QDir::homePath());

	for (const auto& mapping : {
			 qMakePair(tempPath, QStringLiteral("TEMP")),
			 qMakePair(personalAppDataPath, QStringLiteral("APPDATA")),
			 qMakePair(globalAppDataPath, QStringLiteral("GLOBALAPPDATA")),
			 qMakePair(desktopPath, QStringLiteral("DESKTOP")),
			 qMakePair(documentsPath, QStringLiteral("DOCUMENTS")),
			 qMakePair(downloadsPath, QStringLiteral("DOWNLOADS")),
			 qMakePair(picturesPath, QStringLiteral("PICTURES")),
			 qMakePair(videosPath, QStringLiteral("VIDEOS")),
			 qMakePair(homePath, QStringLiteral("HOME")),
		 })
	{
		if (path.startsWith(mapping.first))
		{
			path.replace(mapping.first, QStringLiteral("%%1%").arg(mapping.second));
			break;
		}
	}

	// remove duplicate directory separators - however skip the first two chars
	// as they might specify an UNC path on Windows
	if (path.length() > 3)
	{
		return QDir::toNativeSeparators(path.left(2) + path.mid(2).replace(QString(QStringLiteral("%1%1")).arg(QDir::separator()), QDir::separator()));
	}

	return QDir::toNativeSeparators(path);
}



bool Filesystem::ensurePathExists(const QString& path) const
{
	const QString expandedPath = VeyonCore::filesystem().expandPath(path);

	if (path.isEmpty() || QDir(expandedPath).exists())
	{
		return true;
	}

	vDebug() << "creating " << path << "=>" << expandedPath;

	QString p = expandedPath;

	QStringList dirs;
	while (!QDir(p).exists() && !p.isEmpty())
	{
		dirs.push_front(QDir(p).dirName());
		p.chop(dirs.front().size() + 1);
	}

	if (!p.isEmpty())
	{
		return QDir(p).mkpath(dirs.join(QDir::separator()));
	}

	return false;
}



QString Filesystem::privateKeyPath(const QString& name) const
{
	const QString d = VeyonCore::filesystem().expandPath(VeyonCore::config().privateKeyBaseDir()) +
		QDir::separator() + name + QDir::separator() + QStringLiteral("key");

	return QDir::toNativeSeparators(d);
}



QString Filesystem::publicKeyPath(const QString& name) const
{
	const QString d = VeyonCore::filesystem().expandPath(VeyonCore::config().publicKeyBaseDir()) +
		QDir::separator() + name + QDir::separator() + QStringLiteral("key");

	return QDir::toNativeSeparators(d);
}



QString Filesystem::screenshotDirectoryPath() const
{
	return expandPath(VeyonCore::config().screenshotDirectory());
}



QString Filesystem::serverFilePath() const
{
	return QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + QDir::separator() +
									QStringLiteral("veyon-server") + VeyonCore::executableSuffix());
}



QString Filesystem::workerFilePath() const
{
	return QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + QDir::separator() +
									QStringLiteral("veyon-worker") + VeyonCore::executableSuffix());
}
