// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>

#include <QDir>
#include <QFileInfo>

#include <aclapi.h>
#include <sddl.h>

#include "WindowsLogFileDirectory.h"
#include "WindowsSmartObjects.h"

#include <array>
#include <string>
#include <utility>


namespace {

using SidBuffer = std::array<BYTE, SECURITY_MAX_SID_SIZE>;

std::wstring nativePath(const QString& path)
{
	return QDir::toNativeSeparators(path).toStdWString();
}



bool createWellKnownSid(WELL_KNOWN_SID_TYPE type, SidBuffer& buffer)
{
	auto size = DWORD(buffer.size());
	return CreateWellKnownSid(type, nullptr, buffer.data(), &size) != FALSE;
}



bool isTokenMember(WELL_KNOWN_SID_TYPE type)
{
	SidBuffer sid{};
	BOOL member = FALSE;
	return createWellKnownSid(type, sid) && CheckTokenMembership(nullptr, sid.data(), &member) != FALSE &&
		member != FALSE;
}

}



QString WindowsLogFileDirectory::systemDirectorySecurityDescriptor()
{
	// owned by Administrators; SYSTEM and Administrators full access (inherited by files and
	// subdirectories), nothing else
	return QStringLiteral("O:BAD:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)");
}



QString WindowsLogFileDirectory::parentDirectorySecurityDescriptor()
{
	// owned by Administrators independent of the default owner of the token, inherited DACL
	return QStringLiteral("O:BA");
}



bool WindowsLogFileDirectory::isPrivilegedProcess()
{
	return isTokenMember(WinLocalSystemSid) || isTokenMember(WinBuiltinAdministratorsSid);
}



WindowsLogFileDirectory::Result WindowsLogFileDirectory::prepareSystemDirectory(const QString& path)
{
	// %ProgramData%\Veyon: users may create subdirectories in %ProgramData%, so an existing one has
	// to be owned by SYSTEM or the Administrators group as well
	const auto parentPath = QFileInfo(path).absolutePath();
	SmartSecurityDescriptor parentSecurityDescriptor;
	if (const auto error = createSecurityDescriptor(parentDirectorySecurityDescriptor(), parentSecurityDescriptor);
		error.isEmpty() == false)
	{
		return {{}, error};
	}

	SECURITY_ATTRIBUTES parentAttributes{};
	parentAttributes.nLength = sizeof(parentAttributes);
	parentAttributes.lpSecurityDescriptor = parentSecurityDescriptor.get();
	parentAttributes.bInheritHandle = FALSE;

	const auto nativeParentPath = nativePath(parentPath);
	if (CreateDirectoryW(nativeParentPath.c_str(), &parentAttributes) == FALSE)
	{
		if (const auto error = GetLastError(); error != ERROR_ALREADY_EXISTS)
		{
			return {{}, QStringLiteral("could not create directory %1: error %2").arg(parentPath).arg(error)};
		}
	}

	for (const auto& error : {checkDirectory(parentPath), checkOwner(parentPath)})
	{
		if (error.isEmpty() == false)
		{
			return {{}, error};
		}
	}

	SmartSecurityDescriptor securityDescriptor;
	if (const auto error = createSecurityDescriptor(systemDirectorySecurityDescriptor(), securityDescriptor);
		error.isEmpty() == false)
	{
		return {{}, error};
	}

	SECURITY_ATTRIBUTES attributes{};
	attributes.nLength = sizeof(attributes);
	attributes.lpSecurityDescriptor = securityDescriptor.get();
	attributes.bInheritHandle = FALSE;

	const auto directoryPath = nativePath(path);
	const auto created = CreateDirectoryW(directoryPath.c_str(), &attributes) != FALSE;
	if (created == false)
	{
		if (const auto error = GetLastError(); error != ERROR_ALREADY_EXISTS)
		{
			return {{}, QStringLiteral("could not create log directory %1: error %2").arg(path).arg(error)};
		}
	}

	QStringList errors{checkDirectory(path), checkOwner(path)};
	if (created == false && errors.join(QString()).isEmpty())
	{
		// an existing directory may have been created with an inherited DACL
		errors.append(applySecurityDescriptor(path, securityDescriptor.get()));
	}
	errors.append(checkCanonicalPath(path));

	for (const auto& error : std::as_const(errors))
	{
		if (error.isEmpty() == false)
		{
			return {{}, error};
		}
	}

	return {QDir::cleanPath(path), {}};
}



WindowsLogFileDirectory::Result WindowsLogFileDirectory::prepareUserDirectory(const QString& path)
{
	if (path.isEmpty() || QDir::isAbsolutePath(path) == false)
	{
		return {{}, QStringLiteral("log directory \"%1\" is not an absolute path").arg(path)};
	}

	const auto directoryPath = nativePath(path);
	if (CreateDirectoryW(directoryPath.c_str(), nullptr) == FALSE)
	{
		if (const auto error = GetLastError(); error != ERROR_ALREADY_EXISTS)
		{
			return {{}, QStringLiteral("could not create log directory %1: error %2").arg(path).arg(error)};
		}
	}

	for (const auto& error : {checkDirectory(path), checkCanonicalPath(path)})
	{
		if (error.isEmpty() == false)
		{
			return {{}, error};
		}
	}

	return {QDir::cleanPath(path), {}};
}



QString WindowsLogFileDirectory::checkDirectory(const QString& path)
{
	const auto attributes = GetFileAttributesW(nativePath(path).c_str());
	if (attributes == INVALID_FILE_ATTRIBUTES)
	{
		const auto error = GetLastError();
		return QStringLiteral("could not query log directory %1: error %2").arg(path).arg(error);
	}

	if (attributes & FILE_ATTRIBUTE_REPARSE_POINT)
	{
		return QStringLiteral("log directory %1 is a symlink or junction").arg(path);
	}

	if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		return QStringLiteral("log directory %1 is not a directory").arg(path);
	}

	return {};
}



QString WindowsLogFileDirectory::checkCanonicalPath(const QString& path)
{
	const QFileInfo fileInfo(path);
	if (fileInfo.canonicalFilePath().compare(fileInfo.absoluteFilePath(), Qt::CaseInsensitive) != 0)
	{
		return QStringLiteral("log directory %1 or one of its parent directories is a symlink or junction").arg(path);
	}

	return {};
}



QString WindowsLogFileDirectory::checkOwner(const QString& path)
{
	PSID owner = nullptr;
	SmartSecurityDescriptor securityDescriptor;
	const auto result = GetNamedSecurityInfoW(nativePath(path).c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
											  &owner, nullptr, nullptr, nullptr, securityDescriptor.put());
	if (result != ERROR_SUCCESS || owner == nullptr)
	{
		return QStringLiteral("could not query owner of directory %1: error %2").arg(path).arg(result);
	}

	if (IsWellKnownSid(owner, WinLocalSystemSid) || IsWellKnownSid(owner, WinBuiltinAdministratorsSid))
	{
		return {};
	}

	return QStringLiteral("directory %1 is not owned by SYSTEM or the Administrators group").arg(path);
}



QString WindowsLogFileDirectory::applySecurityDescriptor(const QString& path, PSECURITY_DESCRIPTOR securityDescriptor)
{
	BOOL daclPresent = FALSE;
	BOOL daclDefaulted = FALSE;
	PACL dacl = nullptr;
	if (GetSecurityDescriptorDacl(securityDescriptor, &daclPresent, &dacl, &daclDefaulted) == FALSE ||
		daclPresent == FALSE)
	{
		const auto error = GetLastError();
		return QStringLiteral("could not get DACL for log directory %1: error %2").arg(path).arg(error);
	}

	auto directoryPath = nativePath(path);
	const auto result = SetNamedSecurityInfoW(directoryPath.data(), SE_FILE_OBJECT,
											  DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
											  nullptr, nullptr, dacl, nullptr);
	if (result != ERROR_SUCCESS)
	{
		return QStringLiteral("could not set DACL of log directory %1: error %2").arg(path).arg(result);
	}

	return {};
}



QString WindowsLogFileDirectory::createSecurityDescriptor(const QString& securityDescriptorString,
														  SmartSecurityDescriptor& securityDescriptor)
{
	const auto string = securityDescriptorString.toStdWString();
	if (ConvertStringSecurityDescriptorToSecurityDescriptorW(string.c_str(), SDDL_REVISION_1, securityDescriptor.put(),
															 nullptr) == FALSE)
	{
		const auto error = GetLastError();
		return QStringLiteral("could not create security descriptor %1: error %2").arg(securityDescriptorString).arg(error);
	}

	return {};
}
