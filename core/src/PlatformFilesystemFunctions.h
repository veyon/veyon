// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFile>

#include "PlatformCoreFunctions.h"
#include "PlatformPluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class PlatformFilesystemFunctions
{
public:
	// result of prepareLogFileDirectory(): the directory to create log files in or, if it can't be
	// used safely, an empty path and the reason
	struct LogFileDirectory
	{
		QString path;
		QString error;
	};

	virtual QString personalAppDataPath() const = 0;
	virtual QString globalAppDataPath() const = 0;
	virtual QString globalTempPath() const = 0;

	virtual QString fileOwnerGroup(const QString& filePath) = 0;
	virtual bool setFileOwnerGroup(const QString& filePath, const QString& ownerGroup) = 0;
	virtual bool setFileOwnerGroupPermissions(const QString& filePath, QFile::Permissions permissions) = 0;

	virtual bool openFileSafely(QFile* file, QFile::OpenMode openMode, QFile::Permissions permissions) = 0;

	// directory for log files of privileged processes (Linux: /var/log/veyon, Windows:
	// %ProgramData%\Veyon\logs)
	virtual QString systemLogFileDirectory() const = 0;

	// directory for log files of unprivileged processes of the given user below the configured
	// (expanded) log file directory (Linux: <base>/veyon-<login name>, Windows: <base>); with "*" as
	// login name, the result is a pattern matching the directories of all users
	virtual QString userLogFileDirectory(const QString& baseDirectory, const QString& loginName) const = 0;

	// whether this process logs to systemLogFileDirectory() instead of a user log file directory,
	// decided by the effective uid (Linux) or the token (Windows: SYSTEM or elevated)
	virtual bool usesSystemLogFileDirectory() const = 0;

	// creates or validates the log file directory of this process; fails closed with the reason
	[[nodiscard]] virtual LogFileDirectory prepareLogFileDirectory(const QString& baseDirectory) = 0;

	virtual PlatformCoreFunctions::ProcessId findFileLockingProcess(const QString& filePath) const = 0;
};
