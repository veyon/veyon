/*
 * WindowsFilesystemFunctions.cpp - implementation of WindowsFilesystemFunctions class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <QDir>

#include <shlobj.h>
#include <accctrl.h>
#include <aclapi.h>

#include "WindowsCoreFunctions.h"
#include "WindowsFilesystemFunctions.h"


static QString windowsConfigPath( REFKNOWNFOLDERID folderId )
{
	QString result;

	wchar_t* path = nullptr;
	if( SHGetKnownFolderPath( folderId, KF_FLAG_DEFAULT, nullptr, &path ) == S_OK )
	{
		result = QString::fromWCharArray( path );
		CoTaskMemFree( path );
	}

	return result;
}



QString WindowsFilesystemFunctions::personalAppDataPath() const
{
	return windowsConfigPath( FOLDERID_RoamingAppData ) + QDir::separator() + QStringLiteral("Veyon");
}



QString WindowsFilesystemFunctions::globalAppDataPath() const
{
	return windowsConfigPath( FOLDERID_ProgramData ) + QDir::separator() + QStringLiteral("Veyon");
}



QString WindowsFilesystemFunctions::globalTempPath() const
{
	return windowsConfigPath( FOLDERID_Windows ) + QDir::separator() + QStringLiteral("Temp");
}



QString WindowsFilesystemFunctions::fileOwnerGroup( const QString& filePath )
{
	PSID ownerSID = nullptr;
	PSECURITY_DESCRIPTOR securityDescriptor = nullptr;

	const auto secInfoResult = GetNamedSecurityInfo( WindowsCoreFunctions::toConstWCharArray( filePath ),
													 SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
													 &ownerSID, nullptr, nullptr, nullptr, &securityDescriptor );
	if( secInfoResult != ERROR_SUCCESS )
	{
		qCritical() << Q_FUNC_INFO << "GetSecurityInfo() failed:" << secInfoResult;
		return QString();
	}

	DWORD nameSize = 0;
	DWORD domainSize = 0;
	SID_NAME_USE sidNameUse = SidTypeUnknown;

	LookupAccountSid( nullptr, ownerSID, nullptr, &nameSize, nullptr, &domainSize, &sidNameUse );

	if( nameSize == 0 || domainSize == 0)
	{
		qCritical() << Q_FUNC_INFO << "Failed to retrieve buffer sizes:" << GetLastError();
		return QString();
	}

	wchar_t* name = new wchar_t[nameSize];
	wchar_t* domain = new wchar_t[domainSize];

	if( LookupAccountSid( nullptr, ownerSID, name, &nameSize, domain, &domainSize, &sidNameUse ) == false )
	{
		qCritical() << Q_FUNC_INFO << "LookupAccountSid() (2) failed:" << GetLastError();
		return QString();
	}

	const auto owner = QStringLiteral("%1\\%2").arg( QString::fromWCharArray( domain ), QString::fromWCharArray( name ) );

	delete[] name;
	delete[] domain;

	return owner;
}



bool WindowsFilesystemFunctions::setFileOwnerGroup( const QString& filePath, const QString& ownerGroup )
{
	DWORD sidLen = SECURITY_MAX_SID_SIZE;
	char ownerGroupSID[SECURITY_MAX_SID_SIZE]; // Flawfinder: ignore
	wchar_t domain[PATH_MAX]; // Flawfinder: ignore
	domain[0] = 0;
	DWORD domainLen = PATH_MAX;
	SID_NAME_USE sidNameUse;

	if( LookupAccountName( nullptr, WindowsCoreFunctions::toConstWCharArray( ownerGroup ),
						   ownerGroupSID, &sidLen,
						   domain, &domainLen, &sidNameUse ) == false )
	{
		qCritical() << "Could not look up SID structure:" << GetLastError();
		return false;
	}

	WindowsCoreFunctions::enablePrivilege( SE_TAKE_OWNERSHIP_NAME, true );
	WindowsCoreFunctions::enablePrivilege( SE_RESTORE_NAME, true );

	auto filePathWide = WindowsCoreFunctions::toWCharArray( filePath );

	const auto result = SetNamedSecurityInfo( filePathWide, SE_FILE_OBJECT,
											  OWNER_SECURITY_INFORMATION, ownerGroupSID, nullptr, nullptr, nullptr );

	delete[] filePathWide;

	if( result != ERROR_SUCCESS )
	{
		qCritical() << Q_FUNC_INFO << "SetNamedSecurityInfo() failed:" << result;
	}

	WindowsCoreFunctions::enablePrivilege( SE_TAKE_OWNERSHIP_NAME, false );
	WindowsCoreFunctions::enablePrivilege( SE_RESTORE_NAME, false );

	return result == ERROR_SUCCESS;
}



bool WindowsFilesystemFunctions::setFileOwnerGroupPermissions( const QString& filePath, QFile::Permissions permissions )
{
	PSID ownerSID = nullptr;
	PSECURITY_DESCRIPTOR securityDescriptor = nullptr;

	auto filePathWide = WindowsCoreFunctions::toWCharArray( filePath );

	const auto secInfoResult = GetNamedSecurityInfo( filePathWide, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
													 &ownerSID, nullptr, nullptr, nullptr, &securityDescriptor );

	if( secInfoResult != ERROR_SUCCESS )
	{
		qCritical() << Q_FUNC_INFO << "GetSecurityInfo() failed:" << secInfoResult;
		delete[] filePathWide;
		return false;
	}

	PSID adminSID = nullptr;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	if( AllocateAndInitializeSid( &SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
								  0, 0, 0, 0, 0, 0, &adminSID ) == false )
	{
		delete[] filePathWide;
		return false;
	}

	const int NUM_ACES = 2;
	EXPLICIT_ACCESS ea[NUM_ACES];

	ZeroMemory( &ea, NUM_ACES * sizeof(EXPLICIT_ACCESS) );

	// set read access for owner
	ea[0].grfAccessPermissions = 0;
	if( permissions & QFile::ReadGroup )
	{
		ea[0].grfAccessPermissions |= GENERIC_READ;
	}
	if( permissions & QFile::WriteGroup )
	{
		ea[0].grfAccessPermissions |= GENERIC_WRITE;
	}
	if( permissions & QFile::ExeGroup )
	{
		ea[0].grfAccessPermissions |= GENERIC_EXECUTE;
	}
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[0].Trustee.ptstrName = (LPTSTR) ownerSID;

	// set full control for Administrators
	ea[1].grfAccessPermissions = GENERIC_ALL;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[1].Trustee.ptstrName = (LPTSTR) adminSID;

	PACL acl = nullptr;
	if( SetEntriesInAcl( NUM_ACES, ea, nullptr, &acl ) != ERROR_SUCCESS )
	{
		qCritical() << Q_FUNC_INFO << "SetEntriesInAcl() failed";
		delete[] filePathWide;
		FreeSid( adminSID );
		return false;
	}

	const auto result = SetNamedSecurityInfo( filePathWide, SE_FILE_OBJECT,
											  DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
											  nullptr, nullptr, acl, nullptr );

	if( result != ERROR_SUCCESS )
	{
		qCritical() << Q_FUNC_INFO << "SetNamedSecurityInfo() failed:" << result;
	}

	delete[] filePathWide;
	FreeSid( adminSID );
	LocalFree( acl );

	return result == ERROR_SUCCESS;
}
