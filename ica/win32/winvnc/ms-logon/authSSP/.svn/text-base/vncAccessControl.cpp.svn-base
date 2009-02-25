/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004 Martin Scharpf. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://ultravnc.sourceforge.net/

#include "vncAccessControl.h"

/*
 * GetACL: Gets ACL from reg and puts it in class variable pACL.
 */
PACL
vncAccessControl::GetACL(void){
	HKEY hk = NULL; 
	PACL pInitACL = NULL;
	DWORD dwValueLength = 0;

	__try{
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\ORL\\WinVNC3"),
			0, KEY_QUERY_VALUE, &hk) != ERROR_SUCCESS){
			__leave;
		}

		// Read the ACL value from the VNC registry key
		// First call to RegQueryValueEx just gets the buffer length.
		if (RegQueryValueEx(hk, _T("ACL"), 0, 0, NULL, &dwValueLength) 
			!= ERROR_SUCCESS){
			__leave;
		}
		if (dwValueLength >= sizeof(ACL))
			pInitACL = (PACL) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwValueLength);
		else {
			__leave;
		}
		if (RegQueryValueEx(hk, _T("ACL"), 0, 0, (LPBYTE) pInitACL, &dwValueLength)
			!= ERROR_SUCCESS){
			__leave;
		}

	} __finally {
		if (hk)
			RegCloseKey(hk); 
	}
	return pInitACL;
}



/*
 * GetSD: Accessor function for SecurityDescriptor
 *        Creates an (absolute) SD from the GetACL return value
 */
PSECURITY_DESCRIPTOR
vncAccessControl::GetSD(){
	PSECURITY_DESCRIPTOR pSD;
	PSECURITY_DESCRIPTOR pSelfRelativeSD;
	PACL pACL = NULL;
	DWORD dwBufferLength = 0;

	// If we can't retrieve a valid ACL we create an empty one (i.e. no access).
	if (!(pACL = GetACL()) || !IsValidAcl(pACL)) {
		pACL = (PACL) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,	sizeof(ACL));
		// Initialize the new ACL.
		if (!InitializeAcl(pACL, sizeof(ACL), ACL_REVISION)) {
			; // Todo: Report an error.
		}
	}

	// Construct SD
	pSD = HeapAlloc(GetProcessHeap(), 
		HEAP_ZERO_MEMORY, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if(InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION) &&
		// Set our ACL to the SD.
		SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE) &&
		// AccessCheck() is picky about what is in the SD.
		SetSecurityDescriptorOwner(pSD, GetOwnerSID(), FALSE) &&
		SetSecurityDescriptorGroup(pSD, GetOwnerSID(), FALSE)) {
	} else {
		// Todo: Report an error.
	}
	// Make SD self-relative and use LocalAlloc
	MakeSelfRelativeSD(pSD, NULL, &dwBufferLength);
	pSelfRelativeSD = (PSECURITY_DESCRIPTOR) LocalAlloc(0, dwBufferLength);
	MakeSelfRelativeSD(pSD, pSelfRelativeSD, &dwBufferLength);
	FreeSD(pSD);
	return pSelfRelativeSD;
}

/*
 * SetSD: Changes the class variable pACL and the reg key.
 *        The ACL is the DACL of the provided SD.
 */
BOOL
vncAccessControl::SetSD(PSECURITY_DESCRIPTOR pSD){
	BOOL isOK = FALSE;
	BOOL bDaclPresent = FALSE;
	BOOL bDaclDefaulted = FALSE;
	PACL pDACL = NULL;
	PACL pNewACL = NULL;

	GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pDACL, &bDaclDefaulted);

	if (bDaclPresent && pDACL && IsValidAcl(pDACL) && StoreACL(pDACL))
		isOK = TRUE;
	return isOK;
}

/*
 * GetOwnerSID: Helperfunction for SD creation
 */
PSID
vncAccessControl::GetOwnerSID(void){
	PSID pAdminSid = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
	// Create a SID for the BUILTIN\Administrators group.
	AllocateAndInitializeSid( &SIDAuth, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0, &pAdminSid);
	return pAdminSid;
}

/*
 * StoreACL: Stores the value of the class variable pACL in the registry.
 * Returns TRUE if successful.
 */
BOOL 
vncAccessControl::StoreACL(PACL pACL){
    HKEY hk = NULL; 
	BOOL isSaveOK = FALSE;

    ACL_SIZE_INFORMATION AclInfo = {0, 0, 0};
	DWORD nAclInformationLength = sizeof(AclInfo);

	// Todo: Better error handling
	if (pACL)
		GetAclInformation(pACL, &AclInfo, nAclInformationLength, AclSizeInformation);

	__try{ if (RegCreateKey(HKEY_LOCAL_MACHINE, _T("Software\\ORL\\WinVNC3"), &hk)
			!= ERROR_SUCCESS){
			__leave;
		}
		  if (hk)
		  RegCloseKey(hk);

		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\ORL\\WinVNC3"), 0, KEY_SET_VALUE, &hk)
			!= ERROR_SUCCESS){
			__leave;
		}
		
		if (RegSetValueEx(hk, _T("ACL"), 0, REG_BINARY, (LPBYTE) pACL, AclInfo.AclBytesInUse)
			!= ERROR_SUCCESS){
			__leave;
		}
		isSaveOK = TRUE;
	} __finally {
		if (hk)
			RegCloseKey(hk); 
	}
	return isSaveOK;
} 

/*
 * FreeSD: Frees the memory of an absolute SD.
 */
void
vncAccessControl::FreeSD(PSECURITY_DESCRIPTOR pSD){
	PSID pOwnerSID = NULL;
	PSID pGroupSID = NULL;
	PACL pDACL = NULL;
	PACL pSACL = NULL;
	BOOL bOwnerDefaulted = FALSE;
	BOOL bGroupDefaulted = FALSE;
	BOOL bDaclPresent = FALSE;
	BOOL bDaclDefaulted = FALSE;
	BOOL bSaclPresent = FALSE;
	BOOL bSaclDefaulted = FALSE;
	
	if (pSD) {
		GetSecurityDescriptorOwner(pSD, &pOwnerSID, &bOwnerDefaulted);
		GetSecurityDescriptorGroup(pSD, &pGroupSID, &bGroupDefaulted);
		GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pDACL, &bDaclDefaulted);
		GetSecurityDescriptorSacl(pSD, &bSaclPresent, &pSACL, &bSaclDefaulted);
	}
	// Clean up
	if (pSD)
		HeapFree(GetProcessHeap(), 0, pSD);
	if (bDaclPresent && pDACL)
		HeapFree(GetProcessHeap(), 0, pDACL);
	if (bSaclPresent && pSACL)
		HeapFree(GetProcessHeap(), 0, pSACL);
	if (pOwnerSID)
		HeapFree(GetProcessHeap(), 0, pOwnerSID);
	if (pGroupSID)
		HeapFree(GetProcessHeap(), 0, pGroupSID);

}
