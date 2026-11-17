// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

#include "PlatformFilesystemFunctions.h"
#include "WindowsSmartObjects.h"

// log file directories on Windows: processes running as SYSTEM or with an elevated token log to
// %ProgramData%\Veyon\logs (protected DACL: SYSTEM and Administrators only), all others to the
// configured directory (%TEMP% is per user); a directory that can't be used safely is refused
class WindowsLogFileDirectory
{
public:
	using Result = PlatformFilesystemFunctions::LogFileDirectory;

	// SDDL of the protected DACL of the system log file directory, inherited by the log files
	static QString systemDirectorySecurityDescriptor();

	// SDDL of %ProgramData%\Veyon when created here: owner only, the DACL is inherited
	static QString parentDirectorySecurityDescriptor();

	// whether the token of this process is SYSTEM or a member of the (enabled) Administrators group
	static bool isPrivilegedProcess();

	// creates the directory (and its parent) owned by the Administrators group with the protected
	// DACL or, if it exists and is owned by SYSTEM or the Administrators group, reapplies the DACL;
	// refuses reparse points and any other owner
	[[nodiscard]] static Result prepareSystemDirectory(const QString& path);

	// creates the directory with an inherited DACL if missing; refuses reparse points
	[[nodiscard]] static Result prepareUserDirectory(const QString& path);

private:
	[[nodiscard]] static QString checkDirectory(const QString& path);
	[[nodiscard]] static QString checkCanonicalPath(const QString& path);
	[[nodiscard]] static QString checkOwner(const QString& path);
	[[nodiscard]] static QString createSecurityDescriptor(const QString& securityDescriptorString,
														  SmartSecurityDescriptor& securityDescriptor);
	[[nodiscard]] static QString applySecurityDescriptor(const QString& path, PSECURITY_DESCRIPTOR securityDescriptor);
};
