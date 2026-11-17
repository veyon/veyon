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

	// %TEMP% may contain 8.3 short names, which the canonical path check refuses
	static QString canonicalPath(const QTemporaryDir& directory, const QString& name)
	{
		return QFileInfo(directory.path()).canonicalFilePath() + QLatin1Char('/') + name;
	}

private Q_SLOTS:
	void securityDescriptor()
	{
		QCOMPARE(WindowsLogFileDirectory::systemDirectorySecurityDescriptor(),
				 QStringLiteral("D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)"));
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

		if (WindowsLogFileDirectory::isPrivilegedProcess())
		{
			const auto systemResult = WindowsLogFileDirectory::prepareSystemDirectory(link);
			QVERIFY(systemResult.path.isEmpty());
			QVERIFY2(systemResult.error.contains(QStringLiteral("symlink or junction")), qPrintable(systemResult.error));
		}
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
	}

	void existingSystemDirectoryGetsProtectedDacl()
	{
		if (WindowsLogFileDirectory::isPrivilegedProcess() == false)
		{
			QSKIP("requires an elevated process");
		}

		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		QVERIFY(QDir().mkpath(canonicalPath(directory, QStringLiteral("Veyon/logs"))));
		const auto path = canonicalPath(directory, QStringLiteral("Veyon/logs"));

		// inherited DACL before
		AceList aces;
		QVERIFY(readDirectoryDacl(path, aces) == false);

		const auto result = WindowsLogFileDirectory::prepareSystemDirectory(path);
		QVERIFY2(result.error.isEmpty(), qPrintable(result.error));

		aces.clear();
		QVERIFY(readDirectoryDacl(path, aces));
		QCOMPARE(aces, expectedAces());
	}
};


QTEST_GUILESS_MAIN(WindowsLogFileDirectoryTest)
#include "WindowsLogFileDirectoryTest.moc"
