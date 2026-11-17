// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>

#include <QScopeGuard>
#include <QtTest>

#include "IpcServer.h"

#include <aclapi.h>
#include <algorithm>
#include <sddl.h>
#include <utility>

using IpcPipeAceList = QList<QPair<QString, quint32>>;
Q_DECLARE_METATYPE(IpcPipeAceList)

// DACLs of the IPC endpoint pipes on Windows (02-ipc.md §1, §6a): protected (no inherited ACEs),
// exactly the expected principals and rights, nothing for users, everyone or interactive logons
class IpcPipeSecurityTest : public QObject
{
	Q_OBJECT
private:
	using AceList = IpcPipeAceList;

	static constexpr quint32 GenericAll = GENERIC_ALL;
	static constexpr quint32 FileAllAccess = FILE_ALL_ACCESS;
	static constexpr quint32 CreatePipeInstance = FILE_CREATE_PIPE_INSTANCE;
	static constexpr quint32 UserPipeAccess = 0x12018b;

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

	// returns the ACEs of a DACL, fails the test for anything but plain non-inherited allow ACEs
	static bool readDacl(PACL dacl, AceList& aces)
	{
		if (dacl == nullptr)
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
			if (header->AceType != ACCESS_ALLOWED_ACE_TYPE || (header->AceFlags & (INHERITED_ACE | OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE)) != 0)
			{
				return false;
			}
			const auto allowedAce = static_cast<ACCESS_ALLOWED_ACE*>(ace);
			aces.append({sidToString(&allowedAce->SidStart), quint32(allowedAce->Mask)});
		}

		std::sort(aces.begin(), aces.end());
		return true;
	}

	static AceList sorted(AceList aces)
	{
		std::sort(aces.begin(), aces.end());
		return aces;
	}

	static IpcServer::PeerPolicy policy(IpcServer::WindowsAccess access, const QString& userSid = {})
	{
		IpcServer::PeerPolicy peerPolicy;
		peerPolicy.windowsAccess = access;
		peerPolicy.windowsUserSid = userSid;
		return peerPolicy;
	}

	static QString currentUserSid()
	{
		HANDLE token = nullptr;
		if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) == FALSE)
		{
			return {};
		}
		char buffer[256];
		DWORD size = 0;
		QString sid;
		if (GetTokenInformation(token, TokenUser, buffer, sizeof(buffer), &size))
		{
			sid = sidToString(reinterpret_cast<TOKEN_USER*>(buffer)->User.Sid);
		}
		CloseHandle(token);
		return sid;
	}

private Q_SLOTS:
	void descriptorPerPolicy_data()
	{
		static const auto userSid = QStringLiteral("S-1-5-21-1111111111-2222222222-3333333333-1001");

		QTest::addColumn<int>("access");
		QTest::addColumn<QString>("userSid");
		QTest::addColumn<AceList>("expectedAces");

		QTest::newRow("system-and-administrators")
			<< int(IpcServer::WindowsAccess::SystemAndAdministrators) << QString()
			<< sorted({{QStringLiteral("S-1-5-18"), GenericAll},
					   {QStringLiteral("S-1-5-32-544"), GenericAll},
					   {QStringLiteral("S-1-3-4"), CreatePipeInstance}});
		QTest::newRow("system-only")
			<< int(IpcServer::WindowsAccess::SystemOnly) << QString()
			<< sorted({{QStringLiteral("S-1-5-18"), GenericAll},
					   {QStringLiteral("S-1-3-4"), CreatePipeInstance}});
		QTest::newRow("system-and-user")
			<< int(IpcServer::WindowsAccess::SystemAndUser) << userSid
			<< sorted({{QStringLiteral("S-1-5-18"), GenericAll},
					   {userSid, UserPipeAccess},
					   {QStringLiteral("S-1-3-4"), CreatePipeInstance}});
	}

	void descriptorPerPolicy()
	{
		QFETCH(int, access);
		QFETCH(QString, userSid);
		QFETCH(AceList, expectedAces);

		const auto sddl = IpcServer::windowsPipeSecurityDescriptor(policy(IpcServer::WindowsAccess(access), userSid));
		QVERIFY(sddl.has_value());

		PSECURITY_DESCRIPTOR descriptor = nullptr;
		QVERIFY(ConvertStringSecurityDescriptorToSecurityDescriptorW(reinterpret_cast<LPCWSTR>(sddl->utf16()),
																	 SDDL_REVISION_1, &descriptor, nullptr));
		const auto freeDescriptor = qScopeGuard([descriptor]() { LocalFree(descriptor); });

		SECURITY_DESCRIPTOR_CONTROL control = 0;
		DWORD revision = 0;
		QVERIFY(GetSecurityDescriptorControl(descriptor, &control, &revision));
		QVERIFY(control & SE_DACL_PRESENT);
		QVERIFY(control & SE_DACL_PROTECTED);

		BOOL daclPresent = FALSE;
		BOOL daclDefaulted = FALSE;
		PACL dacl = nullptr;
		QVERIFY(GetSecurityDescriptorDacl(descriptor, &daclPresent, &dacl, &daclDefaulted));
		QVERIFY(daclPresent);

		AceList aces;
		QVERIFY(readDacl(dacl, aces));
		QCOMPARE(aces, expectedAces);

		// the user ACE must not allow adding pipe instances (FILE_CREATE_PIPE_INSTANCE = FILE_APPEND_DATA)
		for (const auto& ace : std::as_const(aces))
		{
			if (ace.first != QLatin1String("S-1-5-18") && ace.first != QLatin1String("S-1-5-32-544") &&
				ace.first != QLatin1String("S-1-3-4"))
			{
				QVERIFY((ace.second & CreatePipeInstance) == 0);
				QVERIFY((ace.second & GENERIC_WRITE) == 0);
				QVERIFY((ace.second & GENERIC_ALL) == 0);
			}
		}
	}

	void invalidUserSid()
	{
		for (const auto& sid : {QString(), QStringLiteral("S-1-5-18;(A;;GA;;;WD)"), QStringLiteral("WD"),
								QStringLiteral("S-1-5-21-1\n"), QStringLiteral("S-1")})
		{
			QVERIFY2(IpcServer::windowsPipeSecurityDescriptor(policy(IpcServer::WindowsAccess::SystemAndUser, sid)).has_value() == false,
					 qPrintable(sid));
		}
	}

	// the DACL as applied by Windows to a real pipe: needs READ_CONTROL, i.e. an administrator
	// (CI runner) for the SystemAndAdministrators policy and the current user for SystemAndUser
	void appliedToPipe_data()
	{
		QTest::addColumn<int>("access");
		QTest::newRow("system-and-administrators") << int(IpcServer::WindowsAccess::SystemAndAdministrators);
		QTest::newRow("system-and-user") << int(IpcServer::WindowsAccess::SystemAndUser);
	}

	void appliedToPipe()
	{
		QFETCH(int, access);

		const auto userSid = currentUserSid();
		QVERIFY(userSid.isEmpty() == false);

		IpcServer server(IpcProtocol::Role::Worker, 0);
		QVERIFY(server.listen({}, policy(IpcServer::WindowsAccess(access), userSid)));

		const auto pipeName = QStringLiteral("\\\\.\\pipe\\%1").arg(server.endpointName());
		const auto handle = CreateFileW(reinterpret_cast<LPCWSTR>(pipeName.utf16()), READ_CONTROL, 0, nullptr,
										OPEN_EXISTING, 0, nullptr);
		if (handle == INVALID_HANDLE_VALUE)
		{
			const auto error = GetLastError();
			if (error == ERROR_ACCESS_DENIED)
			{
				QSKIP("no READ_CONTROL on the pipe (test not running as administrator)");
			}
			QFAIL(qPrintable(QStringLiteral("CreateFileW() failed with %1").arg(error)));
		}
		const auto closeHandle = qScopeGuard([handle]() { CloseHandle(handle); });

		PACL dacl = nullptr;
		PSECURITY_DESCRIPTOR descriptor = nullptr;
		QCOMPARE(GetSecurityInfo(handle, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
								 nullptr, nullptr, &dacl, nullptr, &descriptor),
				 DWORD(ERROR_SUCCESS));
		const auto freeDescriptor = qScopeGuard([descriptor]() { LocalFree(descriptor); });

		SECURITY_DESCRIPTOR_CONTROL control = 0;
		DWORD revision = 0;
		QVERIFY(GetSecurityDescriptorControl(descriptor, &control, &revision));
		QVERIFY(control & SE_DACL_PROTECTED);

		AceList aces;
		QVERIFY(readDacl(dacl, aces));

		// Windows maps GENERIC_ALL to FILE_ALL_ACCESS when applying the descriptor
		AceList expectedAces{{QStringLiteral("S-1-5-18"), FileAllAccess}, {QStringLiteral("S-1-3-4"), CreatePipeInstance}};
		if (IpcServer::WindowsAccess(access) == IpcServer::WindowsAccess::SystemAndAdministrators)
		{
			expectedAces.append({QStringLiteral("S-1-5-32-544"), FileAllAccess});
		}
		else
		{
			expectedAces.append({userSid, UserPipeAccess});
		}
		QCOMPARE(aces, sorted(expectedAces));
	}
};


QTEST_GUILESS_MAIN(IpcPipeSecurityTest)
#include "IpcPipeSecurityTest.moc"
