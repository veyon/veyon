/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "ProcessIdentity.h"

#include <QFile>
#include <QFileInfo>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
QByteArray readFileAt(int directory, const char* name)
{
	const int fd = ::openat(directory, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
	if (fd < 0)
	{
		return {};
	}
	QByteArray data;
	char buffer[4096];
	for (;;)
	{
		const auto count = ::read(fd, buffer, sizeof(buffer));
		if (count < 0 && errno == EINTR)
		{
			continue;
		}
		if (count < 0 || data.size() + count > 65536)
		{
			data.clear();
			break;
		}
		if (count == 0)
		{
			break;
		}
		data.append(buffer, int(count));
	}
	::close(fd);
	return data;
}

bool trustedPath(const QString& executable, const struct stat& image)
{
	if (!S_ISREG(image.st_mode) || image.st_uid != 0 || (image.st_mode & (S_IWGRP | S_IWOTH)))
	{
		return false;
	}
	const QFileInfo file(executable);
	if (!file.isAbsolute() || file.canonicalFilePath() != executable)
	{
		return false;
	}
	// Replacing a root-owned file through a writable directory is also unsafe.
	auto directory = file.absolutePath();
	for (;;)
	{
		struct stat info{};
		if (::stat(QFile::encodeName(directory).constData(), &info) != 0 || !S_ISDIR(info.st_mode) ||
			info.st_uid != 0 || (info.st_mode & (S_IWGRP | S_IWOTH)))
		{
			return false;
		}
		if (directory == QLatin1String("/"))
		{
			return true;
		}
		directory = QFileInfo(directory).absolutePath();
	}
}
}

bool ProcessIdentity::isValid() const
{
	return pid != 0 && startTime != 0 && uid != uint(-1) && inode != 0 && !executable.isEmpty();
}

bool ProcessIdentity::sameProcess(const ProcessIdentity& other) const
{
	return isValid() && other.isValid() && pid == other.pid && parentPid == other.parentPid &&
		uid == other.uid && startTime == other.startTime && device == other.device && inode == other.inode &&
		executable == other.executable;
}

bool ProcessIdentity::isLightDmWrapper(const ProcessIdentity& parent, const ProcessIdentity& helper,
	uint leader, uint greeterUid) const
{
	const auto parentName = QFileInfo(parent.executable).fileName();
	return isValid() && parent.isValid() && helper.isValid() && trustedExecutable &&
		parent.trustedExecutable && helper.trustedExecutable && helper.uid == 0 &&
		greeterUid != 0 && uid == greeterUid &&
		device == helper.device && inode == helper.inode && executable == helper.executable &&
		parentPid == parent.pid && parent.pid == leader && parent.uid == 0 &&
		(parentName == QLatin1String("lightdm") || parentName == QLatin1String(".lightdm-wrapped"));
}

bool ProcessIdentity::isLightDmSessionChildOf(const ProcessIdentity& daemon) const
{
	const auto name = QFileInfo(daemon.executable).fileName();
	return isValid() && daemon.isValid() && uid == 0 && daemon.uid == 0 &&
		trustedExecutable && daemon.trustedExecutable && parentPid == daemon.pid && pid != daemon.pid &&
		startTime >= daemon.startTime && device == daemon.device && inode == daemon.inode &&
		executable == daemon.executable &&
		(name == QLatin1String("lightdm") || name == QLatin1String(".lightdm-wrapped"));
}

bool ProcessIdentity::parseStat(const QByteArray& data, uint& pid, uint& parentPid, quint64& startTime)
{
	// comm may contain spaces and ')'; fields after its last ')' have fixed positions.
	const auto open = data.indexOf('(');
	const auto close = data.lastIndexOf(')');
	if (open < 1 || close <= open)
	{
		return false;
	}
	const auto fields = data.mid(close + 1).simplified().split(' ');
	if (fields.size() < 20)
	{
		return false;
	}
	bool pidOk = false, parentOk = false, timeOk = false;
	pid = data.left(open).trimmed().toUInt(&pidOk);
	parentPid = fields.at(1).toUInt(&parentOk);
	startTime = fields.at(19).toULongLong(&timeOk);
	return pidOk && parentOk && timeOk && pid != 0 && startTime != 0;
}

ProcessIdentity ProcessIdentity::read(uint pid)
{
	ProcessIdentity result;
	const QByteArray path = QByteArrayLiteral("/proc/") + QByteArray::number(pid);
	// An open proc directory refers to this process, rather than a later reuse
	// of its PID. Read all identity fields relative to the same directory FD.
	const int directory = ::open(path.constData(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
	if (directory < 0)
	{
		return {};
	}
	bool ok = parseStat(readFileAt(directory, "stat"), result.pid, result.parentPid, result.startTime) && result.pid == pid;
	const auto status = readFileAt(directory, "status");
	for (const auto& line : status.split('\n'))
	{
		if (!line.startsWith("Uid:"))
		{
			continue;
		}
		const auto fields = line.mid(4).simplified().split(' ');
		if (fields.size() == 4)
		{
			bool uidOk = false;
			result.uid = fields.first().toUInt(&uidOk);
			ok = ok && uidOk;
			for (const auto& field : fields)
			{
				const auto value = field.toUInt(&uidOk);
				ok = ok && uidOk && value == result.uid;
			}
		}
		break;
	}
	char target[4096];
	const auto length = ::readlinkat(directory, "exe", target, sizeof(target));
	struct stat image{}, after{};
	if (length > 0 && size_t(length) < sizeof(target) && ::fstatat(directory, "exe", &image, 0) == 0)
	{
		result.executable = QFile::decodeName(QByteArray(target, int(length)));
		result.device = quint64(image.st_dev);
		result.inode = quint64(image.st_ino);
		result.trustedExecutable = trustedPath(result.executable, image);
	}
	uint afterPid = 0, afterParent = 0;
	quint64 afterStart = 0;
	ok = ok && parseStat(readFileAt(directory, "stat"), afterPid, afterParent, afterStart) &&
		afterPid == result.pid && afterParent == result.parentPid && afterStart == result.startTime &&
		::fstatat(directory, "exe", &after, 0) == 0 && after.st_dev == image.st_dev && after.st_ino == image.st_ino;
	::close(directory);
	return ok && result.isValid() ? result : ProcessIdentity();
}
