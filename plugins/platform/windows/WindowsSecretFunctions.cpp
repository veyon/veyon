// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>
#include <aclapi.h>
#include <dpapi.h>
#include <sddl.h>
#include <shlobj.h>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QScopeGuard>

#include "PlatformCoreFunctions.h"
#include "WindowsSecretFunctions.h"
#include "WindowsSmartObjects.h"
#include "Random.h"

#include <array>

// owner Administrators; SYSTEM and Administrators only, not inherited from the parent; inherited by
// files and directories (the installer creates both directories the same way, nsis/veyon.nsi.in)
static constexpr auto SecretDirectorySecurityDescriptor = L"O:BAD:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)";
static constexpr auto SecretFileSecurityDescriptor = L"O:BAD:P(A;;FA;;;SY)(A;;FA;;;BA)";
// %ProgramData%\Veyon: SYSTEM and Administrators full control, users read and execute
static constexpr auto ParentDirectorySecurityDescriptor =
	L"O:BAD:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;0x1200a9;;;BU)";
static constexpr auto TemporaryFileSuffix = ".tmp";
static constexpr int StaleTemporaryFileAge = 60;


static LPCWSTR wideString(const QString& string)
{
	return reinterpret_cast<LPCWSTR>(string.utf16());
}



static bool isTrustedSid(PSID sid)
{
	return sid && IsValidSid(sid) &&
		(IsWellKnownSid(sid, WinLocalSystemSid) || IsWellKnownSid(sid, WinBuiltinAdministratorsSid));
}



// owner SYSTEM or Administrators; with checkDacl also a protected DACL whose allow ACEs all
// belong to SYSTEM or Administrators
static bool hasTrustedSecurity(HANDLE handle, bool checkDacl)
{
	PSID owner = nullptr;
	PACL dacl = nullptr;
	SmartSecurityDescriptor securityDescriptor;
	if (GetSecurityInfo(handle, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
						&owner, nullptr, &dacl, nullptr, securityDescriptor.put()) != ERROR_SUCCESS ||
		isTrustedSid(owner) == false)
	{
		return false;
	}

	if (checkDacl == false)
	{
		return true;
	}

	SECURITY_DESCRIPTOR_CONTROL control = 0;
	DWORD revision = 0;
	// a NULL DACL grants full access to everyone
	if (dacl == nullptr ||
		GetSecurityDescriptorControl(securityDescriptor.get(), &control, &revision) == FALSE ||
		(control & SE_DACL_PROTECTED) == 0)
	{
		return false;
	}

	for (DWORD i = 0; i < dacl->AceCount; ++i)
	{
		LPVOID ace = nullptr;
		if (GetAce(dacl, i, &ace) == FALSE || ace == nullptr)
		{
			return false;
		}
		const auto header = static_cast<const ACE_HEADER*>(ace);
		if (header->AceType == ACCESS_DENIED_ACE_TYPE)
		{
			continue;
		}
		// any other allow ACE type (object, callback, …) is refused
		if (header->AceType != ACCESS_ALLOWED_ACE_TYPE ||
			isTrustedSid(&static_cast<ACCESS_ALLOWED_ACE*>(ace)->SidStart) == false)
		{
			return false;
		}
	}

	return true;
}



// directory handle without FILE_SHARE_DELETE so that the directory can't be renamed or replaced
// while it is in use; isUntrusted tells an existing directory with wrong security from other errors
static SmartFileHandle openTrustedDirectory(const QString& path, bool checkDacl, bool* isUntrusted = nullptr)
{
	if (isUntrusted)
	{
		*isUntrusted = false;
	}

	SmartFileHandle handle(CreateFileW(wideString(path), READ_CONTROL | FILE_LIST_DIRECTORY | FILE_TRAVERSE,
									   FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
									   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
	if (handle.isInvalid())
	{
		return {};
	}

	BY_HANDLE_FILE_INFORMATION information{};
	if (GetFileInformationByHandle(handle.get(), &information) == FALSE ||
		(information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ||
		(information.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 ||
		hasTrustedSecurity(handle.get(), checkDacl) == false)
	{
		vCritical() << "refusing directory with wrong type, owner or permissions:" << path;
		if (isUntrusted)
		{
			*isUntrusted = true;
		}
		return {};
	}

	return handle;
}



static bool createDirectory(const QString& path, LPCWSTR securityDescriptor)
{
	SECURITY_ATTRIBUTES attributes{};
	attributes.nLength = sizeof(attributes);
	if (ConvertStringSecurityDescriptorToSecurityDescriptorW(securityDescriptor, SDDL_REVISION_1,
															 &attributes.lpSecurityDescriptor, nullptr) == FALSE)
	{
		return false;
	}
	const auto freeDescriptor = qScopeGuard([&attributes]() { LocalFree(attributes.lpSecurityDescriptor); });

	if (CreateDirectoryW(wideString(path), &attributes) == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
	{
		vCritical() << "failed to create directory" << path << "- error" << GetLastError();
		return false;
	}

	return true;
}



static bool setPrivilege(LPCWSTR privilegeName, bool enable)
{
	SmartHandle token;
	LUID luid{};
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, token.put()) == FALSE ||
		LookupPrivilegeValueW(nullptr, privilegeName, &luid) == FALSE)
	{
		return false;
	}

	TOKEN_PRIVILEGES privileges{};
	privileges.PrivilegeCount = 1;
	privileges.Privileges[0].Luid = luid;
	privileges.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

	return AdjustTokenPrivileges(token.get(), FALSE, &privileges, sizeof(privileges), nullptr, nullptr) &&
		GetLastError() == ERROR_SUCCESS;
}



static bool isReparsePoint(const QString& path)
{
	const auto attributes = GetFileAttributesW(wideString(path));
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}



static bool isEmptyDirectory(const QString& path)
{
	WIN32_FIND_DATAW findData{};
	const QString pattern = path + QStringLiteral("\\*");
	const auto findHandle = FindFirstFileW(wideString(pattern), &findData);
	if (findHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	const auto closeFind = qScopeGuard([findHandle]() { FindClose(findHandle); });

	do
	{
		const auto name = QString::fromWCharArray(findData.cFileName);
		if (name != QLatin1String(".") && name != QLatin1String(".."))
		{
			return false;
		}
	} while (FindNextFileW(findHandle, &findData));

	return true;
}



// renames path itself (a reparse point is renamed, never its target); retried because another
// process may hold a handle without FILE_SHARE_DELETE for a moment
static bool moveAside(const QString& path)
{
	static constexpr int Attempts = 5;
	static constexpr DWORD RetryDelayMs = 200;

	for (int attempt = 0; attempt < Attempts; ++attempt)
	{
		const auto randomSuffix = Veyon::Crypto::Random::hex(8);
		if (randomSuffix.has_value() == false)
		{
			return false;
		}
		const QString asidePath = path + QStringLiteral(".untrusted-") + *randomSuffix;
		if (MoveFileExW(wideString(path), wideString(asidePath), 0))
		{
			vWarning() << "moved untrusted directory" << path << "to" << asidePath;
			return true;
		}
		Sleep(RetryDelayMs);
	}

	vCritical() << "failed to move untrusted directory" << path << "aside - error" << GetLastError();
	return false;
}



// takes ownership of the directory itself and replaces its DACL, by handle: the handle is opened
// with FILE_FLAG_OPEN_REPARSE_POINT and refused if it is a reparse point, so no junction or
// symlink is followed (unlike takeown/icacls)
static bool takeOverDirectory(const QString& path, LPCWSTR securityDescriptor)
{
	// open and change the directory regardless of its current DACL and owner (only for this call)
	static constexpr std::array<LPCWSTR, 3> Privileges{L"SeTakeOwnershipPrivilege", L"SeRestorePrivilege",
													   L"SeBackupPrivilege"};
	for (const auto privilege : Privileges)
	{
		setPrivilege(privilege, true);
	}
	const auto disablePrivileges = qScopeGuard([]() {
		for (const auto privilege : Privileges)
		{
			setPrivilege(privilege, false);
		}
	});

	SmartFileHandle handle(CreateFileW(wideString(path), READ_CONTROL | WRITE_DAC | WRITE_OWNER,
									   FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
									   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
	BY_HANDLE_FILE_INFORMATION information{};
	if (handle.isInvalid() ||
		GetFileInformationByHandle(handle.get(), &information) == FALSE ||
		(information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ||
		(information.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
	{
		vCritical() << "can't take over directory" << path << "- error" << GetLastError();
		return false;
	}

	SmartSecurityDescriptor descriptor;
	PSID owner = nullptr;
	PACL dacl = nullptr;
	BOOL ownerDefaulted = FALSE;
	BOOL daclPresent = FALSE;
	BOOL daclDefaulted = FALSE;
	if (ConvertStringSecurityDescriptorToSecurityDescriptorW(securityDescriptor, SDDL_REVISION_1, descriptor.put(),
															 nullptr) == FALSE ||
		GetSecurityDescriptorOwner(descriptor.get(), &owner, &ownerDefaulted) == FALSE ||
		GetSecurityDescriptorDacl(descriptor.get(), &daclPresent, &dacl, &daclDefaulted) == FALSE ||
		daclPresent == FALSE || dacl == nullptr)
	{
		return false;
	}

	const auto result = SetSecurityInfo(handle.get(), SE_FILE_OBJECT,
										OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION |
											PROTECTED_DACL_SECURITY_INFORMATION,
										owner, nullptr, dacl, nullptr);
	if (result != ERROR_SUCCESS)
	{
		vCritical() << "failed to take over directory" << path << "- error" << result;
		return false;
	}

	vWarning() << "took over directory with foreign owner or permissions:" << path;

	return true;
}



// %ProgramData%\Veyon: created if missing; it is never moved (it contains keys\ and more), a
// reparse point in its place is moved aside, a directory with a foreign owner is taken over
static bool ensureParentDirectory(const QString& path)
{
	if (isReparsePoint(path) && moveAside(path) == false)
	{
		return false;
	}

	if (createDirectory(path, ParentDirectorySecurityDescriptor) == false)
	{
		return false;
	}

	bool isUntrusted = false;
	if (openTrustedDirectory(path, false, &isUntrusted).isValid())
	{
		return true;
	}

	return isUntrusted && isReparsePoint(path) == false &&
		takeOverDirectory(path, ParentDirectorySecurityDescriptor) &&
		openTrustedDirectory(path, false).isValid();
}



// the secrets directory: a reparse point or an empty directory with foreign owner or DACL (e.g.
// pre-created by a user to block the store) is moved aside (or, if that fails, an empty one is
// taken over); a foreign directory that already contains files is never replaced silently by an
// empty store
static bool ensureSecretDirectory(const QString& path)
{
	if (isReparsePoint(path) && moveAside(path) == false)
	{
		return false;
	}

	if (createDirectory(path, SecretDirectorySecurityDescriptor) == false)
	{
		return false;
	}

	bool isUntrusted = false;
	if (openTrustedDirectory(path, true, &isUntrusted).isValid())
	{
		return true;
	}
	if (isUntrusted == false)
	{
		return false;
	}

	if (isEmptyDirectory(path) == false)
	{
		const auto message = QStringLiteral("Secret directory %1 has a foreign owner or permissions and contains files - "
											"refusing to use it; check its content and move it aside manually")
								 .arg(QDir::toNativeSeparators(path));
		vCritical() << qUtf8Printable(message);
		// the service keeps running without secrets, which would go unnoticed otherwise
		VeyonCore::platform().coreFunctions().writeToNativeLoggingSystem(message, Logger::LogLevel::Critical);
		return false;
	}

	if (moveAside(path) && createDirectory(path, SecretDirectorySecurityDescriptor) &&
		openTrustedDirectory(path, true).isValid())
	{
		return true;
	}

	// e.g. re-created by someone else in the meantime: take over what is there if it is empty
	return isReparsePoint(path) == false && isEmptyDirectory(path) &&
		takeOverDirectory(path, SecretDirectorySecurityDescriptor) &&
		openTrustedDirectory(path, true).isValid();
}



// temporary files of writers that were killed before the move; recent ones may belong to a
// concurrent writer
static void removeStaleTemporaryFiles(const QString& directory)
{
	const auto now = QDateTime::currentDateTimeUtc();
	const auto files = QDir(directory).entryInfoList(QStringList{QString(QStringLiteral(".*") + QLatin1String(TemporaryFileSuffix))},
													 QDir::Files | QDir::Hidden | QDir::System);
	for (const auto& file : files)
	{
		if (file.isSymLink() == false && file.lastModified().toUTC().secsTo(now) > StaleTemporaryFileAge)
		{
			DeleteFileW(wideString(QDir::toNativeSeparators(file.absoluteFilePath())));
		}
	}
}



WindowsSecretFunctions::WindowsSecretFunctions() :
	m_directory(secretDirectory())
{
#if defined(VEYON_WITH_TESTS)
	m_checkParentDirectory = qEnvironmentVariableIsEmpty("VEYON_SECRET_DIR");
#endif
}



// %ProgramData%\Veyon must belong to SYSTEM or Administrators: its owner could otherwise rename
// or replace the secrets directory
SmartFileHandle WindowsSecretFunctions::openParentDirectory() const
{
	const auto parentDirectory = QFileInfo(m_directory).absolutePath();
	if (m_checkParentDirectory == false)
	{
		return SmartFileHandle(CreateFileW(wideString(parentDirectory), READ_CONTROL, FILE_SHARE_READ | FILE_SHARE_WRITE,
										   nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr));
	}

	return openTrustedDirectory(parentDirectory, false);
}



std::optional<Veyon::Crypto::SecureBuffer> WindowsSecretFunctions::readSecret(const QString& key)
{
	if (m_directory.isEmpty() || QFileInfo::exists(m_directory) == false)
	{
		return {};
	}

	const auto parentHandle = openParentDirectory();
	const auto directoryHandle = openTrustedDirectory(m_directory, true);
	if (parentHandle.isInvalid() || directoryHandle.isInvalid())
	{
		return {};
	}

	const auto path = filePath(key);
	SmartFileHandle file(CreateFileW(wideString(path), GENERIC_READ | READ_CONTROL, FILE_SHARE_READ, nullptr,
									 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
	if (file.isInvalid())
	{
		const auto error = GetLastError();
		if (error != ERROR_FILE_NOT_FOUND)
		{
			vWarning() << "failed to open secret file" << path << "- error" << error;
		}
		return {};
	}

	BY_HANDLE_FILE_INFORMATION information{};
	LARGE_INTEGER fileSize{};
	if (GetFileInformationByHandle(file.get(), &information) == FALSE ||
		(information.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0 ||
		information.nNumberOfLinks != 1 ||
		hasTrustedSecurity(file.get(), true) == false ||
		GetFileSizeEx(file.get(), &fileSize) == FALSE)
	{
		vCritical() << "refusing to read secret file with wrong type, owner or permissions:" << path;
		return {};
	}

	// DPAPI adds less than 1 KiB
	if (fileSize.QuadPart > Veyon::Crypto::SecureBuffer::MaximumSize + 4096)
	{
		vCritical() << "secret file too large:" << path;
		return {};
	}

	QByteArray protectedData(qsizetype(fileSize.QuadPart), Qt::Uninitialized);
	DWORD bytesRead = 0;
	if (ReadFile(file.get(), protectedData.data(), DWORD(protectedData.size()), &bytesRead, nullptr) == FALSE ||
		bytesRead != DWORD(protectedData.size()))
	{
		vCritical() << "failed to read secret file" << path;
		return {};
	}

	DATA_BLOB input{DWORD(protectedData.size()), reinterpret_cast<BYTE*>(protectedData.data())};
	DATA_BLOB output{};
	if (CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output) == FALSE)
	{
		vCritical() << "failed to unprotect secret file" << path << "- error" << GetLastError();
		return {};
	}

	Veyon::Crypto::SecureBuffer value(reinterpret_cast<const char*>(output.pbData), qsizetype(output.cbData));
	SecureZeroMemory(output.pbData, output.cbData);
	LocalFree(output.pbData);

	return value;
}



bool WindowsSecretFunctions::writeSecret(const QString& key, const Veyon::Crypto::SecureBuffer& value)
{
	if (prepareStorage() == false)
	{
		return false;
	}

	const auto parentHandle = openParentDirectory();
	const auto directoryHandle = openTrustedDirectory(m_directory, true);
	if (parentHandle.isInvalid() || directoryHandle.isInvalid())
	{
		return false;
	}

	DATA_BLOB input{DWORD(value.size()), const_cast<BYTE*>(reinterpret_cast<const BYTE*>(value.constData()))};
	DATA_BLOB output{};
	if (CryptProtectData(&input, L"Veyon secret", nullptr, nullptr, nullptr,
						 CRYPTPROTECT_LOCAL_MACHINE | CRYPTPROTECT_UI_FORBIDDEN, &output) == FALSE)
	{
		vCritical() << "failed to protect secret" << key << "- error" << GetLastError();
		return false;
	}
	const auto freeOutput = qScopeGuard([&output]() { LocalFree(output.pbData); });

	const auto randomSuffix = Veyon::Crypto::Random::hex(8);
	if (randomSuffix.has_value() == false)
	{
		return false;
	}

	const auto path = filePath(key);
	const QString temporaryPath = m_directory + QLatin1Char('\\') + QLatin1Char('.') + QFileInfo(path).fileName() +
		QLatin1Char('.') + *randomSuffix + QLatin1String(TemporaryFileSuffix);

	SECURITY_ATTRIBUTES fileAttributes{};
	fileAttributes.nLength = sizeof(fileAttributes);
	if (ConvertStringSecurityDescriptorToSecurityDescriptorW(SecretFileSecurityDescriptor, SDDL_REVISION_1,
															 &fileAttributes.lpSecurityDescriptor, nullptr) == FALSE)
	{
		return false;
	}
	const auto freeFileDescriptor = qScopeGuard([&fileAttributes]() { LocalFree(fileAttributes.lpSecurityDescriptor); });

	auto success = false;
	{
		SmartFileHandle file(CreateFileW(wideString(temporaryPath), GENERIC_WRITE, 0, &fileAttributes, CREATE_NEW,
										 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr));
		if (file.isInvalid())
		{
			vCritical() << "failed to create temporary secret file in" << m_directory << "- error" << GetLastError();
			return false;
		}

		DWORD bytesWritten = 0;
		success = WriteFile(file.get(), output.pbData, output.cbData, &bytesWritten, nullptr) &&
			bytesWritten == output.cbData &&
			FlushFileBuffers(file.get());
	}

	if (success == false ||
		MoveFileExW(wideString(temporaryPath), wideString(path), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE)
	{
		vCritical() << "failed to write secret file" << path << "- error" << GetLastError();
		DeleteFileW(wideString(temporaryPath));
		return false;
	}

	return true;
}



bool WindowsSecretFunctions::removeSecret(const QString& key)
{
	if (m_directory.isEmpty())
	{
		return false;
	}

	if (QFileInfo::exists(m_directory) == false)
	{
		return true;
	}

	const auto parentHandle = openParentDirectory();
	const auto directoryHandle = openTrustedDirectory(m_directory, true);
	if (parentHandle.isInvalid() || directoryHandle.isInvalid())
	{
		return false;
	}

	const auto path = filePath(key);
	if (DeleteFileW(wideString(path)) == FALSE && GetLastError() != ERROR_FILE_NOT_FOUND)
	{
		vCritical() << "failed to remove secret file" << path << "- error" << GetLastError();
		return false;
	}

	return true;
}



QStringList WindowsSecretFunctions::secretKeys()
{
	if (m_directory.isEmpty() || QFileInfo::exists(m_directory) == false)
	{
		return {};
	}

	const auto directoryHandle = openTrustedDirectory(m_directory, true);
	if (directoryHandle.isInvalid())
	{
		return {};
	}

	QStringList keys;
	const auto names = QDir(m_directory).entryList(QDir::Files | QDir::Hidden | QDir::System);
	for (const auto& name : names)
	{
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



bool WindowsSecretFunctions::prepareStorage()
{
	if (m_directory.isEmpty())
	{
		return false;
	}

	const auto parentDirectory = QFileInfo(m_directory).absolutePath();
	if ((m_checkParentDirectory ? ensureParentDirectory(parentDirectory) : createDirectory(parentDirectory, ParentDirectorySecurityDescriptor)) == false ||
		ensureSecretDirectory(m_directory) == false)
	{
		return false;
	}

	removeStaleTemporaryFiles(m_directory);

	return true;
}



bool WindowsSecretFunctions::isAvailable()
{
	// SYSTEM (service) or an elevated administrator (configurator, CLI)
	return m_directory.isEmpty() == false &&
		VeyonCore::platform().coreFunctions().isRunningAsAdmin();
}



QString WindowsSecretFunctions::secretDirectory() const
{
#if defined(VEYON_WITH_TESTS)
	const auto testDirectory = qEnvironmentVariable("VEYON_SECRET_DIR");
	if (testDirectory.isEmpty() == false)
	{
		return QDir::toNativeSeparators(QDir::cleanPath(testDirectory));
	}
#endif

	QString programDataPath;
	SmartCoTaskMemPtr<wchar_t> path;
	if (SHGetKnownFolderPath(FOLDERID_ProgramData, KF_FLAG_DEFAULT, nullptr, path.put()) == S_OK)
	{
		programDataPath = QString::fromWCharArray(path.get());
	}

	// empty (all operations fail) if the known folder can't be determined
	if (programDataPath.isEmpty())
	{
		return {};
	}

	return QDir::toNativeSeparators(programDataPath + QStringLiteral("/Veyon/secrets"));
}



QString WindowsSecretFunctions::filePath(const QString& key) const
{
	// keys are validated by SecretStore: one "/", ASCII only, scopes never contain "_"
	return m_directory + QLatin1Char('\\') + QString(key).replace(QLatin1Char('/'), QLatin1String("__"));
}
