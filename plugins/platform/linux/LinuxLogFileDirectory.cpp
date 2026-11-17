// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDir>
#include <QFile>

#include "LinuxLogFileDirectory.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <optional>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>


namespace {

// "veyon-" plus the login name must fit into one path component (NAME_MAX = 255 bytes)
constexpr int MaximumLoginNameSize = 249;
constexpr size_t PasswordBufferSize = 16384;

QString modeString(mode_t mode)
{
	return QStringLiteral("0%1").arg(mode & 07777, 0, 8);
}



QString errnoString()
{
	return QString::fromLocal8Bit(strerror(errno));
}



QString ownerError(const QString& path, uid_t owner, uid_t expectedOwner)
{
	return QStringLiteral("log directory %1 is owned by uid %2 instead of %3")
		.arg(path)
		.arg(owner)
		.arg(expectedOwner == 0 ? QStringLiteral("root") : QString::number(expectedOwner));
}



// opens a directory without following a symlink as the last component; explains a failure by the
// type and owner of the directory entry
QString openDirectory(const QString& path, int* descriptor, struct stat* status,
					  std::optional<uid_t> expectedOwner = std::nullopt)
{
	*descriptor = ::open(QFile::encodeName(path).constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
	if (*descriptor < 0)
	{
		const auto openError = errno;
		struct stat linkStatus{};
		if (::lstat(QFile::encodeName(path).constData(), &linkStatus) == 0)
		{
			if (S_ISLNK(linkStatus.st_mode))
			{
				return QStringLiteral("log directory %1 is a symlink").arg(path);
			}
			if (expectedOwner.has_value() && S_ISDIR(linkStatus.st_mode) && linkStatus.st_uid != *expectedOwner)
			{
				return ownerError(path, linkStatus.st_uid, *expectedOwner);
			}
		}
		if (openError == ENOTDIR)
		{
			return QStringLiteral("log directory %1 is not a directory").arg(path);
		}
		errno = openError;
		return QStringLiteral("could not open log directory %1: %2").arg(path, errnoString());
	}

	if (::fstat(*descriptor, status) != 0)
	{
		const auto error = QStringLiteral("could not stat log directory %1: %2").arg(path, errnoString());
		::close(*descriptor);
		*descriptor = -1;
		return error;
	}

	return {};
}

}



QString LinuxLogFileDirectory::systemDirectory()
{
	return QStringLiteral("/var/log/veyon");
}



QString LinuxLogFileDirectory::loginName(uid_t userId)
{
	struct passwd entry{};
	struct passwd* result = nullptr;
	QByteArray buffer(PasswordBufferSize, 0);

	if (::getpwuid_r(userId, &entry, buffer.data(), size_t(buffer.size()), &result) != 0 || result == nullptr ||
		result->pw_name == nullptr)
	{
		return {};
	}

	return QString::fromLocal8Bit(result->pw_name);
}



QString LinuxLogFileDirectory::sanitizedLoginName(const QString& loginName, uid_t userId)
{
	const auto isSafe = [&loginName]() {
		if (loginName.isEmpty() || QFile::encodeName(loginName).size() > MaximumLoginNameSize ||
			loginName.contains(QLatin1Char('/')) || loginName.contains(QLatin1String("..")) ||
			loginName == QLatin1String("."))
		{
			return false;
		}

		for (const auto character : loginName)
		{
			if (character.category() == QChar::Other_Control || character.unicode() == 0x7f)
			{
				return false;
			}
		}

		return true;
	};

	if (isSafe())
	{
		return loginName;
	}

	return QStringLiteral("uid-%1").arg(userId);
}



QString LinuxLogFileDirectory::userDirectory(const QString& baseDirectory, const QString& loginName, uid_t userId)
{
	// the sanitized login name contains neither separators nor ".."
	return QDir::cleanPath(baseDirectory + QStringLiteral("/veyon-") + sanitizedLoginName(loginName, userId));
}



LinuxLogFileDirectory::Result LinuxLogFileDirectory::prepareSystemDirectory(const QString& path)
{
	bool created = false;
	if (const auto error = createDirectory(path, SystemDirectoryMode, &created); error.isEmpty() == false)
	{
		return {{}, error};
	}

	int descriptor = -1;
	struct stat status{};
	if (const auto error = openDirectory(path, &descriptor, &status, 0); error.isEmpty() == false)
	{
		return {{}, error};
	}

	QString error;
	if (status.st_uid != 0)
	{
		error = ownerError(path, status.st_uid, 0);
	}
	else if (status.st_mode & (S_IWGRP | S_IWOTH))
	{
		error = QStringLiteral("log directory %1 has mode %2 and is writable by group or others")
					.arg(path, modeString(status.st_mode));
	}
	// do not depend on the umask
	else if (created && ::fchmod(descriptor, SystemDirectoryMode) != 0)
	{
		error = QStringLiteral("could not set mode of log directory %1: %2").arg(path, errnoString());
	}

	::close(descriptor);

	if (error.isEmpty())
	{
		error = checkCanonicalPath(path);
	}

	if (error.isEmpty() == false)
	{
		return {{}, error};
	}

	return {QDir::cleanPath(path), {}};
}



LinuxLogFileDirectory::Result LinuxLogFileDirectory::prepareUserDirectory(const QString& baseDirectory,
																		  const QString& loginName, uid_t userId)
{
	if (baseDirectory.isEmpty() || QDir::isAbsolutePath(baseDirectory) == false)
	{
		return {{}, QStringLiteral("log directory \"%1\" is not an absolute path").arg(baseDirectory)};
	}

	// the base directory is created for the user only if missing, an existing one keeps its mode
	bool baseCreated = false;
	if (const auto error = createDirectory(baseDirectory, BaseDirectoryMode, &baseCreated); error.isEmpty() == false)
	{
		return {{}, error};
	}

	int descriptor = -1;
	struct stat status{};
	if (const auto error = openDirectory(baseDirectory, &descriptor, &status); error.isEmpty() == false)
	{
		return {{}, error};
	}

	QString baseError;
	// others must not be able to replace the per-user directory once it has been checked
	if ((status.st_mode & (S_IWGRP | S_IWOTH)) && (status.st_mode & S_ISVTX) == 0)
	{
		baseError = QStringLiteral("log directory %1 has mode %2 and is writable by group or others without the "
								   "sticky bit (set it with chmod +t or use another directory)")
						.arg(baseDirectory, modeString(status.st_mode));
	}
	else if (status.st_uid != 0 && status.st_uid != userId)
	{
		baseError = QStringLiteral("log directory %1 is owned by uid %2 instead of root or %3")
						.arg(baseDirectory)
						.arg(status.st_uid)
						.arg(userId);
	}
	else if (baseCreated && ::fchmod(descriptor, BaseDirectoryMode) != 0)
	{
		baseError = QStringLiteral("could not set mode of log directory %1: %2").arg(baseDirectory, errnoString());
	}
	::close(descriptor);

	if (baseError.isEmpty())
	{
		baseError = checkCanonicalPath(baseDirectory);
	}

	if (baseError.isEmpty() == false)
	{
		return {{}, baseError};
	}

	const auto path = userDirectory(baseDirectory, loginName, userId);

	// the mode is checked and set below whether the directory was created or not
	if (const auto error = createDirectory(path, UserDirectoryMode, nullptr); error.isEmpty() == false)
	{
		return {{}, error};
	}

	if (const auto error = openDirectory(path, &descriptor, &status, userId); error.isEmpty() == false)
	{
		return {{}, error};
	}

	QString error;
	if (status.st_uid != userId)
	{
		error = ownerError(path, status.st_uid, userId);
	}
	// do not depend on the umask or on an earlier mode chosen by the user
	else if ((status.st_mode & 07777) != UserDirectoryMode && ::fchmod(descriptor, UserDirectoryMode) != 0)
	{
		error = QStringLiteral("could not set mode of log directory %1: %2").arg(path, errnoString());
	}

	::close(descriptor);

	if (error.isEmpty())
	{
		error = checkCanonicalPath(path);
	}

	if (error.isEmpty() == false)
	{
		return {{}, error};
	}

	return {path, {}};
}



QString LinuxLogFileDirectory::createDirectory(const QString& path, mode_t mode, bool* created)
{
	if (created)
	{
		*created = false;
	}

	if (path.isEmpty() || QDir::isAbsolutePath(path) == false)
	{
		return QStringLiteral("log directory \"%1\" is not an absolute path").arg(path);
	}

	if (::mkdir(QFile::encodeName(path).constData(), mode) == 0)
	{
		if (created)
		{
			*created = true;
		}
		return {};
	}

	if (errno == EEXIST)
	{
		return {};
	}

	return QStringLiteral("could not create log directory %1: %2").arg(path, errnoString());
}



QString LinuxLogFileDirectory::checkCanonicalPath(const QString& path)
{
	char canonicalPath[PATH_MAX]{};
	if (::realpath(QFile::encodeName(path).constData(), canonicalPath) == nullptr)
	{
		return QStringLiteral("could not resolve log directory %1: %2").arg(path, errnoString());
	}

	if (QFile::decodeName(canonicalPath) != QDir::cleanPath(path))
	{
		return QStringLiteral("log directory %1 or one of its parent directories is a symlink").arg(path);
	}

	return {};
}
