// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

#include "PlatformSecretFunctions.h"
#include "WindowsSmartObjects.h"

#include <veyonconfig.h>

// clazy:excludeall=copyable-polymorphic

// SecretStore backend (03-secretstore-config.md §2): one file "<scope>__<name>" per key in
// %ProgramData%\Veyon\secrets. The directory and the files have a protected DACL granting access to
// SYSTEM and Administrators only (the actual access control); the file content is additionally
// wrapped with DPAPI in the machine scope (defence in depth). Directories and files whose owner or
// DACL grants anything to others, and reparse points, are refused. Writes are atomic (temporary
// file in the same directory, flush, MoveFileEx with MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH).
class WindowsSecretFunctions : public PlatformSecretFunctions
{
public:
	// %ProgramData%\Veyon\secrets or, in builds with tests only, the directory in VEYON_SECRET_DIR
	WindowsSecretFunctions();

	std::optional<Veyon::Crypto::SecureBuffer> readSecret(const QString& key) override;
	bool writeSecret(const QString& key, const Veyon::Crypto::SecureBuffer& value) override;
	bool removeSecret(const QString& key) override;
	QStringList secretKeys() override;
	bool isAvailable() override;
	bool prepareStorage() override;

private:
	SmartFileHandle openParentDirectory() const;
	QString secretDirectory() const;
	QString filePath(const QString& key) const;

	QString m_directory;
	bool m_checkParentDirectory = true;
};
