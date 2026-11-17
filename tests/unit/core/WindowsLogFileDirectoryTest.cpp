// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>

#include <QProcess>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>

#include "WindowsLogFileDirectory.h"

#include <aclapi.h>
#include <algorithm>
#include <array>
#include <sddl.h>

// log file directories on Windows (WP23): %ProgramData%\Veyon\logs with a protected DACL (SYSTEM and
// Administrators only) for SYSTEM/elevated processes, the configured directory for all others; the
// system directory is passed in, so the real %ProgramData% is never touched
class WindowsLogFileDirectoryTest : public QObject
{
	Q_OBJECT
private:
	using AceList = QList<QPair<QString, quint32>>;

	static constexpr quint32 FileAllAccess = FILE_ALL_ACCESS;
	static constexpr BYTE InheritFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;

	static QString sidToString(PSID sid)
	{
		LPWSTR string = nullptr;
		if (ConvertSidToStringSidW(sid, &string) == FALSE)
		{
			return {};
		}
		const auto result = QString::fromWCharArray(string);
		LocalFree(string);
		return result;
	}

	// protected DACL of the directory with non-inherited allow ACEs, inherited by files and directories
	static bool readDirectoryDacl(const QString& path, AceList& aces)
	{
		PACL dacl = nullptr;
		PSECURITY_DESCRIPTOR descriptor = nullptr;
		const auto nativePath = QDir::toNativeSeparators(path).toStdWString();
		if (GetNamedSecurityInfoW(nativePath.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr,
								  &dacl, nullptr, &descriptor) != ERROR_SUCCESS)
		{
			return false;
		}
		const auto freeDescriptor = qScopeGuard([descriptor]() { LocalFree(descriptor); });

		SECURITY_DESCRIPTOR_CONTROL control = 0;
		DWORD revision = 0;
		if (GetSecurityDescriptorControl(descriptor, &control, &revision) == FALSE || (control & SE_DACL_PROTECTED) == 0 ||
			dacl == nullptr)
		{
			return false;
		}

		for (DWORD i = 0; i < dacl->AceCount; ++i)
		{
			void* ace = nullptr;
			if (GetAce(dacl, i, &ace) == FALSE)
			{
				return false;
			}
			const auto header = static_cast<ACE_HEADER*>(ace);
			if (header->AceType != ACCESS_ALLOWED_ACE_TYPE || (header->AceFlags & INHERITED_ACE) != 0 ||
				(header->AceFlags & InheritFlags) != InheritFlags)
			{
				return false;
			}
			const auto allowedAce = static_cast<ACCESS_ALLOWED_ACE*>(ace);
			aces.append({sidToString(&allowedAce->SidStart), quint32(allowedAce->Mask)});
		}

		std::sort(aces.begin(), aces.end());
		return true;
	}

	static AceList expectedAces()
	{
		AceList aces{{QStringLiteral("S-1-5-18"), FileAllAccess}, {QStringLiteral("S-1-5-32-544"), FileAllAccess}};
		std::sort(aces.begin(), aces.end());
		return aces;
	}

	static bool createJunction(const QString& link, const QString& target)
	{
		return QProcess::execute(QStringLiteral("cmd"), {QStringLiteral("/c"), QStringLiteral("mklink"), QStringLiteral("/J"), QDir::toNativeSeparators(link), QDir::toNativeSeparators(target)}) == 0;
	}

	static QString ownerOf(const QString& path)
	{
		PSID owner = nullptr;
		PSECURITY_DESCRIPTOR descriptor = nullptr;
		const auto nativePath = QDir::toNativeSeparators(path).toStdWString();
		if (GetNamedSecurityInfoW(nativePath.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &owner, nullptr,
								  nullptr, nullptr, &descriptor) != ERROR_SUCCESS)
		{
			return {};
		}
		const auto freeDescriptor = qScopeGuard([descriptor]() { LocalFree(descriptor); });
		return sidToString(owner);
	}

	static QString currentUserSid()
	{
		HANDLE token = nullptr;
		if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) == FALSE)
		{
			return {};
		}
		alignas(TOKEN_USER) std::array<BYTE, sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE> buffer{};
		DWORD size = 0;
		QString sid;
		if (GetTokenInformation(token, TokenUser, buffer.data(), DWORD(buffer.size()), &size))
		{
			sid = sidToString(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid);
		}
		CloseHandle(token);
		return sid;
	}

	// creates a directory with the given security descriptor (e.g. an owner only)
	static bool createDirectory(const QString& path, const QString& securityDescriptorString)
	{
		PSECURITY_DESCRIPTOR descriptor = nullptr;
		const auto string = securityDescriptorString.toStdWString();
		if (ConvertStringSecurityDescriptorToSecurityDescriptorW(string.c_str(), SDDL_REVISION_1, &descriptor,
																 nullptr) == FALSE)
		{
			return false;
		}
		const auto freeDescriptor = qScopeGuard([descriptor]() { LocalFree(descriptor); });

		SECURITY_ATTRIBUTES attributes{};
		attributes.nLength = sizeof(attributes);
		attributes.lpSecurityDescriptor = descriptor;
		const auto nativePath = QDir::toNativeSeparators(path).toStdWString();
		return CreateDirectoryW(nativePath.c_str(), &attributes) != FALSE;
	}

	static QString administratorsSid()
	{
		return QStringLiteral("S-1-5-32-544");
	}

	// %TEMP% may contain 8.3 short names, which the canonical path check refuses
	static QString canonicalPath(const QTemporaryDir& directory, const QString& name)
	{
		return QFileInfo(directory.path()).canonicalFilePath() + QLatin1Char('/') + name;
	}

private Q_SLOTS:
	void securityDescriptor()
	{
		QCOMPARE(WindowsLogFileDirectory::systemDirectorySecurityDescriptor(),
				 QStringLiteral("O:BAD:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)"));
		QCOMPARE(WindowsLogFileDirectory::parentDirectorySecurityDescriptor(), QStringLiteral("O:BA"));
	}

	void createsUserDirectory()
	{
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		const auto path = canonicalPath(directory, QStringLiteral("logs"));

		const auto result = WindowsLogFileDirectory::prepareUserDirectory(path);
		QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
		QCOMPARE(result.path, path);
		QVERIFY(QFileInfo(path).isDir());

		// idempotent
		QCOMPARE(WindowsLogFileDirectory::prepareUserDirectory(path).path, path);
	}

	void invalidUserDirectoryRefused()
	{
		for (const auto& path : {QString(), QStringLiteral("logs")})
		{
			const auto result = WindowsLogFileDirectory::prepareUserDirectory(path);
			QVERIFY(result.path.isEmpty());
			QVERIFY2(result.error.contains(QStringLiteral("not an absolute path")), qPrintable(result.error));
		}

		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		const auto result = WindowsLogFileDirectory::prepareUserDirectory(canonicalPath(directory, QStringLiteral("missing/logs")));
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("could not create log directory")), qPrintable(result.error));
	}

	void junctionRefused()
	{
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		const auto target = canonicalPath(directory, QStringLiteral("target"));
		const auto link = canonicalPath(directory, QStringLiteral("logs"));
		QVERIFY(QDir().mkdir(target));
		if (createJunction(link, target) == false)
		{
			QSKIP("could not create a junction");
		}

		const auto result = WindowsLogFileDirectory::prepareUserDirectory(link);
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("symlink or junction")), qPrintable(result.error));

		if (WindowsLogFileDirectory::isPrivilegedProcess() == false)
		{
			return;
		}

		// %ProgramData%\Veyon\logs as a junction
		const auto parent = canonicalPath(directory, QStringLiteral("Veyon"));
		QVERIFY(createDirectory(parent, QStringLiteral("O:BA")));
		const auto systemLink = parent + QStringLiteral("/logs");
		QVERIFY(createJunction(systemLink, target));

		const auto systemResult = WindowsLogFileDirectory::prepareSystemDirectory(systemLink);
		QVERIFY(systemResult.path.isEmpty());
		QVERIFY2(systemResult.error.contains(QStringLiteral("symlink or junction")), qPrintable(systemResult.error));
	}

	void createsSystemDirectoryWithProtectedDacl()
	{
		if (WindowsLogFileDirectory::isPrivilegedProcess() == false)
		{
			QSKIP("requires an elevated process");
		}

		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		// stands for %ProgramData%\Veyon\logs
		const auto path = canonicalPath(directory, QStringLiteral("Veyon/logs"));

		const auto result = WindowsLogFileDirectory::prepareSystemDirectory(path);
		QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
		QCOMPARE(result.path, path);

		AceList aces;
		QVERIFY(readDirectoryDacl(path, aces));
		QCOMPARE(aces, expectedAces());

		// owned by Administrators independent of the default owner of the token
		QCOMPARE(ownerOf(path), administratorsSid());
		QCOMPARE(ownerOf(canonicalPath(directory, QStringLiteral("Veyon"))), administratorsSid());
	}

	void existingSystemDirectoryGetsProtectedDacl()
	{
		if (WindowsLogFileDirectory::isPrivilegedProcess() == false)
		{
			QSKIP("requires an elevated process");
		}

		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		// as created by an administrator: owned by Administrators, inherited DACL
		QVERIFY(createDirectory(canonicalPath(directory, QStringLiteral("Veyon")), QStringLiteral("O:BA")));
		const auto path = canonicalPath(directory, QStringLiteral("Veyon/logs"));
		QVERIFY(createDirectory(path, QStringLiteral("O:BA")));

		// inherited DACL before
		AceList aces;
		QVERIFY(readDirectoryDacl(path, aces) == false);

		const auto result = WindowsLogFileDirectory::prepareSystemDirectory(path);
		QVERIFY2(result.error.isEmpty(), qPrintable(result.error));

		aces.clear();
		QVERIFY(readDirectoryDacl(path, aces));
		QCOMPARE(aces, expectedAces());
	}

	void systemDirectoryOfOtherOwnerRefused()
	{
		if (WindowsLogFileDirectory::isPrivilegedProcess() == false)
		{
			QSKIP("requires an elevated process");
		}

		const auto userSid = currentUserSid();
		QVERIFY(userSid.isEmpty() == false);

		// logs created by the same account without elevation (owner = the user)
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		const auto parent = canonicalPath(directory, QStringLiteral("Veyon"));
		QVERIFY(createDirectory(parent, QStringLiteral("O:BA")));
		const auto path = parent + QStringLiteral("/logs");
		QVERIFY(createDirectory(path, QStringLiteral("O:%1").arg(userSid)));
		QCOMPARE(ownerOf(path), userSid);

		auto result = WindowsLogFileDirectory::prepareSystemDirectory(path);
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("is not owned by SYSTEM or the Administrators group")),
				 qPrintable(result.error));
		// not changed
		AceList aces;
		QVERIFY(readDirectoryDacl(path, aces) == false);

		// the same for %ProgramData%\Veyon
		const auto userParent = canonicalPath(directory, QStringLiteral("VeyonOfUser"));
		QVERIFY(createDirectory(userParent, QStringLiteral("O:%1").arg(userSid)));
		result = WindowsLogFileDirectory::prepareSystemDirectory(userParent + QStringLiteral("/logs"));
		QVERIFY(result.path.isEmpty());
		QVERIFY2(result.error.contains(QStringLiteral("is not owned by SYSTEM or the Administrators group")),
				 qPrintable(result.error));
		QVERIFY(QFileInfo::exists(userParent + QStringLiteral("/logs")) == false);
	}
};


QTEST_GUILESS_MAIN(WindowsLogFileDirectoryTest)
#include "WindowsLogFileDirectoryTest.moc"
