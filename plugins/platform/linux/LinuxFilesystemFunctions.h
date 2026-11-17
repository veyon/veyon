// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PlatformFilesystemFunctions.h"

// clazy:excludeall=copyable-polymorphic

class LinuxFilesystemFunctions : public PlatformFilesystemFunctions
{
public:
	QString personalAppDataPath() const override;
	QString globalAppDataPath() const override;
	QString globalTempPath() const override;

	QString fileOwnerGroup(const QString& filePath) override;
	bool setFileOwnerGroup(const QString& filePath, const QString& ownerGroup) override;
	bool setFileOwnerGroupPermissions(const QString& filePath, QFile::Permissions permissions) override;

	bool openFileSafely(QFile* file, QFile::OpenMode openMode, QFile::Permissions permissions) override;

	QString systemLogFileDirectory() const override;
	QString userLogFileDirectory(const QString& baseDirectory, const QString& loginName) const override;
	bool usesSystemLogFileDirectory() const override;
	LogFileDirectory prepareLogFileDirectory(const QString& baseDirectory) override;

	PlatformCoreFunctions::ProcessId findFileLockingProcess(const QString& filePath) const override;
};
