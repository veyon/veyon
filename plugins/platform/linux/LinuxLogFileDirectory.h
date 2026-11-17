// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PlatformFilesystemFunctions.h"

#include <sys/types.h>

// log file directories on Linux: processes running as root log to /var/log/veyon (owned by root,
// 0750), all others to <base>/veyon-<login name> (owned by the user, 0700); both are created if
// missing and refused instead of being used if they can't be used safely
class LinuxLogFileDirectory
{
public:
	using Result = PlatformFilesystemFunctions::LogFileDirectory;

	static constexpr mode_t SystemDirectoryMode = 0750;
	static constexpr mode_t UserDirectoryMode = 0700;
	static constexpr mode_t BaseDirectoryMode = 0700;

	static QString systemDirectory();

	// login name of the given uid from the user database (empty if unknown)
	static QString loginName(uid_t userId);

	// the login name if it can be used as a path component, otherwise uid-<userId>
	static QString sanitizedLoginName(const QString& loginName, uid_t userId);

	static QString userDirectory(const QString& baseDirectory, const QString& loginName, uid_t userId);

	// creates the directory (owned by root, 0750) or validates an existing one (owned by root, not
	// writable by group or others, no symlink)
	[[nodiscard]] static Result prepareSystemDirectory(const QString& path);

	// creates or validates the base directory (only created if missing, 0700, mode of an existing
	// one is not changed) and the per-user directory below it (owned by userId, 0700, no symlink)
	[[nodiscard]] static Result prepareUserDirectory(const QString& baseDirectory, const QString& loginName,
													 uid_t userId);

private:
	[[nodiscard]] static QString createDirectory(const QString& path, mode_t mode, bool* created);
	[[nodiscard]] static QString checkCanonicalPath(const QString& path);
};
