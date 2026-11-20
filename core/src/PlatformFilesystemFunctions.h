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
	virtual QString personalAppDataPath() const = 0;
	virtual QString globalAppDataPath() const = 0;
	virtual QString globalTempPath() const = 0;

	virtual QString fileOwnerGroup(const QString& filePath) = 0;
	virtual bool setFileOwnerGroup(const QString& filePath, const QString& ownerGroup) = 0;
	virtual bool setFileOwnerGroupPermissions(const QString& filePath, QFile::Permissions permissions) = 0;

	// opens the file through a native descriptor and refuses symlinks and non-regular files (on Linux also
	// files of other users); QFile::fileName() is empty afterwards, so callers have to keep the path
	// themselves and must not use fileName(), remove() or rename() on the QFile object
	virtual bool openFileSafely(QFile* file, QFile::OpenMode openMode, QFile::Permissions permissions) = 0;

	virtual PlatformCoreFunctions::ProcessId findFileLockingProcess(const QString& filePath) const = 0;
};
