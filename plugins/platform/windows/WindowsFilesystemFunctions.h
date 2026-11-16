// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PlatformFilesystemFunctions.h"

// clazy:excludeall=copyable-polymorphic

class WindowsFilesystemFunctions : public PlatformFilesystemFunctions
{
public:
	QString personalAppDataPath() const override;
	QString globalAppDataPath() const override;
	QString globalTempPath() const override;

	QString fileOwnerGroup(const QString& filePath) override;
	bool setFileOwnerGroup(const QString& filePath, const QString& ownerGroup) override;
	bool setFileOwnerGroupPermissions(const QString& filePath, QFile::Permissions permissions) override;

	bool openFileSafely(QFile* file, QFile::OpenMode openMode, QFile::Permissions permissions) override;

	PlatformCoreFunctions::ProcessId findFileLockingProcess(const QString& filePath) const override;
};
