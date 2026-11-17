// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QFileInfo>
#include <QScopeGuard>

#include "LinuxSecretFunctions.h"
#include "Random.h"

#include <cerrno>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static constexpr mode_t SecretDirectoryMode = 0700;
static constexpr mode_t SecretFileMode = 0600;
static constexpr mode_t ParentDirectoryMode = 0755;
static constexpr auto TemporaryFileSuffix = ".tmp";
static constexpr time_t StaleTemporaryFileAge = 60;


LinuxSecretFunctions::LinuxSecretFunctions() :
	m_directory(QString::fromLatin1(DefaultSecretDirectory)),
	m_ownerUserId(0)
{
#if defined(VEYON_WITH_TESTS)
	const auto testDirectory = qEnvironmentVariable("VEYON_SECRET_DIR");
	if (testDirectory.isEmpty() == false)
	{
		m_directory = testDirectory;
		m_ownerUserId = geteuid();
	}
#endif
}



LinuxSecretFunctions::LinuxSecretFunctions(const QString& directory, uid_t ownerUserId) :
	m_directory(directory),
	m_ownerUserId(ownerUserId)
{
}



std::optional<Veyon::Crypto::SecureBuffer> LinuxSecretFunctions::readSecret(const QString& key)
{
	const auto directoryFd = openDirectory(false);
	if (directoryFd < 0)
	{
		return {};
	}
	const auto closeDirectory = qScopeGuard([directoryFd]() { close(directoryFd); });

	const auto name = fileName(key);
	const auto fd = openat(directoryFd, name.constData(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_NOCTTY | O_CLOEXEC);
	if (fd < 0)
	{
		if (errno != ENOENT)
		{
			vWarning() << "refusing to read secret file" << m_directory << name << "- error" << errno;
		}
		return {};
	}
	const auto closeFile = qScopeGuard([fd]() { close(fd); });

	struct stat status{};
	if (fstat(fd, &status) != 0 ||
		S_ISREG(status.st_mode) == false ||
		status.st_nlink != 1 ||
		isPrivateToOwner(status) == false)
	{
		vCritical() << "refusing to read secret file with wrong type, owner or permissions:" << m_directory << name;
		return {};
	}

	if (status.st_size > Veyon::Crypto::SecureBuffer::MaximumSize)
	{
		vCritical() << "secret file too large:" << m_directory << name;
		return {};
	}

	Veyon::Crypto::SecureBuffer value(qsizetype(status.st_size));
	qsizetype bytesRead = 0;
	while (bytesRead < value.size())
	{
		const auto result = read(fd, value.data() + bytesRead, size_t(value.size() - bytesRead));
		if (result < 0 && errno == EINTR)
		{
			continue;
		}
		if (result <= 0)
		{
			vCritical() << "failed to read secret file" << m_directory << name;
			return {};
		}
		bytesRead += result;
	}

	return value;
}



bool LinuxSecretFunctions::writeSecret(const QString& key, const Veyon::Crypto::SecureBuffer& value)
{
	const auto directoryFd = openDirectory(true);
	if (directoryFd < 0)
	{
		return false;
	}
	const auto closeDirectory = qScopeGuard([directoryFd]() { close(directoryFd); });

	removeStaleTemporaryFiles(directoryFd);

	const auto randomSuffix = Veyon::Crypto::Random::hex(8);
	if (randomSuffix.has_value() == false)
	{
		return false;
	}

	const auto name = fileName(key);
	const QByteArray temporaryName = '.' + name + '.' + randomSuffix->toLatin1() + TemporaryFileSuffix;

	const auto fd = openat(directoryFd, temporaryName.constData(),
						   O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_NOCTTY | O_CLOEXEC, SecretFileMode);
	if (fd < 0)
	{
		vCritical() << "failed to create temporary secret file in" << m_directory << "- error" << errno;
		return false;
	}

	auto removeTemporaryFile = qScopeGuard([directoryFd, temporaryName]() {
		unlinkat(directoryFd, temporaryName.constData(), 0);
	});

	auto success = fchmod(fd, SecretFileMode) == 0;

	qsizetype bytesWritten = 0;
	while (success && bytesWritten < value.size())
	{
		const auto result = write(fd, value.constData() + bytesWritten, size_t(value.size() - bytesWritten));
		if (result < 0 && errno == EINTR)
		{
			continue;
		}
		success = result > 0;
		bytesWritten += qMax<qsizetype>(result, 0);
	}

	success = success && fsync(fd) == 0;
	success = (close(fd) == 0) && success;

	if (success == false)
	{
		vCritical() << "failed to write secret file in" << m_directory;
		return false;
	}

#if defined(VEYON_WITH_TESTS)
	if (m_beforeRenameHook)
	{
		m_beforeRenameHook();
	}
#endif

	if (renameat(directoryFd, temporaryName.constData(), directoryFd, name.constData()) != 0)
	{
		vCritical() << "failed to replace secret file" << m_directory << name << "- error" << errno;
		return false;
	}

	removeTemporaryFile.dismiss();

	if (fsync(directoryFd) != 0)
	{
		vWarning() << "failed to sync secret directory" << m_directory;
	}

	return true;
}



bool LinuxSecretFunctions::removeSecret(const QString& key)
{
	const auto directoryFd = openDirectory(false);
	if (directoryFd < 0)
	{
		// nothing stored yet
		return QFileInfo::exists(m_directory) == false;
	}
	const auto closeDirectory = qScopeGuard([directoryFd]() { close(directoryFd); });

	if (unlinkat(directoryFd, fileName(key).constData(), 0) != 0 && errno != ENOENT)
	{
		vCritical() << "failed to remove secret file in" << m_directory << "- error" << errno;
		return false;
	}

	fsync(directoryFd);

	return true;
}



QStringList LinuxSecretFunctions::secretKeys()
{
	const auto directoryFd = openDirectory(false);
	if (directoryFd < 0)
	{
		return {};
	}

	// closedir() closes directoryFd
	const auto directory = fdopendir(directoryFd);
	if (directory == nullptr)
	{
		close(directoryFd);
		return {};
	}
	const auto closeDirectory = qScopeGuard([directory]() { closedir(directory); });

	QStringList keys;
	while (const auto entry = readdir(directory))
	{
		const auto name = QString::fromLatin1(entry->d_name);
		const auto separatorIndex = name.indexOf(QLatin1String("__"));
		// temporary files start with "." and scopes never do
		if (name.startsWith(QLatin1Char('.')) || separatorIndex <= 0)
		{
			continue;
		}

		keys.append(name.left(separatorIndex) + QLatin1Char('/') + name.mid(separatorIndex + 2));
	}

	return keys;
}



bool LinuxSecretFunctions::prepareStorage()
{
	// creates a missing directory; an existing one with wrong type, owner or mode is refused (and
	// logged) by openDirectory(): root owns /etc/veyon, so nobody else can have created it
	const auto directoryFd = openDirectory(true);
	if (directoryFd < 0)
	{
		return false;
	}

	removeStaleTemporaryFiles(directoryFd);
	close(directoryFd);

	return true;
}



bool LinuxSecretFunctions::isAvailable()
{
	return geteuid() == m_ownerUserId;
}



int LinuxSecretFunctions::openDirectory(bool create) const
{
	const auto path = QFile::encodeName(m_directory);

	if (create)
	{
		const auto parentPath = QFile::encodeName(QFileInfo(m_directory).absolutePath());
		if (mkdir(parentPath.constData(), ParentDirectoryMode) != 0 && errno != EEXIST)
		{
			vCritical() << "failed to create directory" << parentPath << "- error" << errno;
			return -1;
		}
		if (mkdir(path.constData(), SecretDirectoryMode) != 0 && errno != EEXIST)
		{
			vCritical() << "failed to create secret directory" << m_directory << "- error" << errno;
			return -1;
		}
	}

	const auto fd = open(path.constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
	if (fd < 0)
	{
		if (errno != ENOENT)
		{
			vCritical() << "refusing to open secret directory" << m_directory << "- error" << errno;
		}
		return -1;
	}

	struct stat status{};
	if (fstat(fd, &status) != 0 ||
		S_ISDIR(status.st_mode) == false ||
		isPrivateToOwner(status) == false)
	{
		vCritical() << "refusing secret directory with wrong owner or permissions:" << m_directory;
		close(fd);
		return -1;
	}

	return fd;
}



// temporary files of writers that were killed before the rename; recent ones may belong to a
// concurrent writer
void LinuxSecretFunctions::removeStaleTemporaryFiles(int directoryFd) const
{
	const auto fd = dup(directoryFd);
	const auto directory = fd >= 0 ? fdopendir(fd) : nullptr;
	if (directory == nullptr)
	{
		if (fd >= 0)
		{
			close(fd);
		}
		return;
	}
	const auto closeDirectory = qScopeGuard([directory]() { closedir(directory); });

	const auto now = time(nullptr);
	while (const auto entry = readdir(directory))
	{
		const auto name = QByteArray(entry->d_name);
		struct stat status{};
		if (name.startsWith('.') && name.endsWith(TemporaryFileSuffix) &&
			fstatat(directoryFd, entry->d_name, &status, AT_SYMLINK_NOFOLLOW) == 0 &&
			S_ISREG(status.st_mode) && status.st_mtime + StaleTemporaryFileAge < now)
		{
			unlinkat(directoryFd, entry->d_name, 0);
		}
	}
}



bool LinuxSecretFunctions::isPrivateToOwner(const struct stat& status) const
{
	return status.st_uid == m_ownerUserId && (status.st_mode & 077) == 0;
}



QByteArray LinuxSecretFunctions::fileName(const QString& key)
{
	// keys are validated by SecretStore: one "/", ASCII only, scopes never contain "_"
	return QString(key).replace(QLatin1Char('/'), QLatin1String("__")).toLatin1();
}
