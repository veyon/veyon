/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004 Martin Scharpf, B. Braun Melsungen AG. All Rights Reserved.
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
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
// /macine-vnc Greg Wood (wood@agressiv.com)
#include "authSSP.h"

/*
 *  AuthSSP.cpp: Domainuser could be 'domain\user', just 'user' or
 *  UPN-style 'user@domain'. Should work with Windows NT 4 and better.
 *  NT 4 does not support UPN-style names.
 */

Fn fn;

BOOL CUPSD2(const char * domainuser, 
		  const char *password, 
		  PSECURITY_DESCRIPTOR psdSD,
		  PBOOL isAuthenticated,
		  PDWORD pdwAccessGranted)	// returns bitmask with accessrights
{
	char domain[MAXLEN];
	const char *user = 0;
	domain[0] = '\0';
	TCHAR user2[MAXLEN];
	TCHAR domain2[MAXLEN];
	TCHAR password2[MAXLEN];

	user = SplitString(domainuser,'\\',domain);

#if defined (_UNICODE) || defined (UNICODE)
	mbstowcs(user2, user, MAXLEN);
	mbstowcs(domain2, domain, MAXLEN);
	mbstowcs(password2, password, MAXLEN);
#else
	strcpy(user2, user);
	strcpy(domain2, domain);
	strcpy(password2, password);
#endif

	// On NT4, prepend computer- or domainname if username is unqualified.
	if (isNT4() && _tcscmp(domain2, _T("")) == 0) {
		if (!QualifyName(user2, domain2)) {
			_tcscpy(domain2, _T("\0"));
		}
	}
	return SSPLogonUser(domain2, user2, password2, psdSD, isAuthenticated, pdwAccessGranted);
}


BOOL WINAPI SSPLogonUser(LPTSTR szDomain, 
						 LPTSTR szUser, 
						 LPTSTR szPassword, 
						 PSECURITY_DESCRIPTOR psdSD,
						 PBOOL isAuthenticated,
						 PDWORD pdwAccessGranted)	// returns bitmask with accessrights
{
	AUTH_SEQ    asServer   = {0};
	AUTH_SEQ    asClient   = {0};
	BOOL        fDone      = FALSE;
	BOOL        fResult    = FALSE;
	DWORD       cbOut      = 0;
	DWORD       cbIn       = 0;
	DWORD       cbMaxToken = 0;
	PVOID       pClientBuf = NULL;
	PVOID       pServerBuf = NULL;
	PSecPkgInfo pSPI       = NULL;
	HMODULE     hModule    = NULL;
	SEC_WINNT_AUTH_IDENTITY ai;

#define __try
#define __leave goto cleanup
#define __finally cleanup:
	__try {
		
		hModule = LoadSecurityDll();
		if (!hModule)
			__leave;
		
		// Get max token size
		fn._QuerySecurityPackageInfo((SEC_CHAR *)_T("NTLM"), &pSPI);
		cbMaxToken = pSPI->cbMaxToken;
		
		// Allocate buffers for client and server messages
		pClientBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbMaxToken);
		pServerBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbMaxToken);
		
		// Initialize auth identity structure
		// Marscha 2004: Seems to work with szDomain = "" or even szDomain = "anyDomain", 
		// but I found no MS documentation for this 'feature'.
		ZeroMemory(&ai, sizeof(ai));
#if defined(UNICODE) || defined(_UNICODE)
		ai.Domain = (unsigned short *)szDomain;
		ai.DomainLength = lstrlen(szDomain);
		ai.User = (unsigned short *)szUser;
		ai.UserLength = lstrlen(szUser);
		ai.Password = (unsigned short *)szPassword;
		ai.PasswordLength = lstrlen(szPassword);
		ai.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
#else      
		ai.Domain = (unsigned char *)szDomain;
		ai.DomainLength = lstrlen(szDomain);
		ai.User = (unsigned char *)szUser;
		ai.UserLength = lstrlen(szUser);
		ai.Password = (unsigned char *)szPassword;
		ai.PasswordLength = lstrlen(szPassword);
		ai.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
#endif

		// Prepare client message (negotiate) .
		cbOut = cbMaxToken;
		if (!GenClientContext(&asClient, &ai, NULL, 0, pClientBuf, &cbOut, &fDone))
			__leave;
		
		// Prepare server message (challenge) .
		cbIn = cbOut;
		cbOut = cbMaxToken;
		if (!GenServerContext(&asServer, pClientBuf, cbIn, pServerBuf, &cbOut, 
            &fDone))
			__leave;
		// Most likely failure: AcceptServerContext fails with SEC_E_LOGON_DENIED
		// in the case of bad szUser or szPassword.
		// Unexpected Result: Logon will succeed if you pass in a bad szUser and 
		// the guest account is enabled in the specified domain.
		
		// Prepare client message (authenticate) .
		cbIn = cbOut;
		cbOut = cbMaxToken;
		if (!GenClientContext(&asClient, &ai, pServerBuf, cbIn, pClientBuf, &cbOut,
            &fDone))
			__leave;
		
		// Prepare server message (authentication) .
		cbIn = cbOut;
		cbOut = cbMaxToken;
		if (!GenServerContext(&asServer, pClientBuf, cbIn, pServerBuf, &cbOut, 
            &fDone))
			__leave;
		
		*isAuthenticated = TRUE;

		// Check authorization
		if (IsImpersonationAllowed()) {
			if (ImpersonateAndCheckAccess(&(asServer.hctxt), psdSD, pdwAccessGranted))
				fResult = TRUE;
		} else {
			// Todo: Make alternative access check
			if (ImpersonateAndCheckAccess(&(asServer.hctxt), psdSD, pdwAccessGranted))
				fResult = TRUE;
		}

	} __finally {

		// Clean up resources
		if (pSPI)
			fn._FreeContextBuffer(pSPI);
		
		if (asClient.fHaveCtxtHandle)
			fn._DeleteSecurityContext(&asClient.hctxt);
		
		if (asClient.fHaveCredHandle)
			fn._FreeCredentialsHandle(&asClient.hcred);
		
		if (asServer.fHaveCtxtHandle)
			fn._DeleteSecurityContext(&asServer.hctxt);
		
		if (asServer.fHaveCredHandle)
			fn._FreeCredentialsHandle(&asServer.hcred);
		
		if (hModule)
			UnloadSecurityDll(hModule);
		
		HeapFree(GetProcessHeap(), 0, pClientBuf);
		HeapFree(GetProcessHeap(), 0, pServerBuf);
		SecureZeroMemory(&ai, sizeof(ai));
	}

	return fResult;
}

BOOL ImpersonateAndCheckAccess(PCtxtHandle phContext, 
							   PSECURITY_DESCRIPTOR psdSD, 
							   PDWORD pdwAccessGranted) {
	HANDLE hToken = NULL;
	
	// AccessCheck() variables
	DWORD           dwAccessDesired = MAXIMUM_ALLOWED;
	PRIVILEGE_SET   PrivilegeSet;
	DWORD           dwPrivSetSize = sizeof(PRIVILEGE_SET);
	BOOL            fAccessGranted = FALSE;
	GENERIC_MAPPING GenericMapping = { vncGenericRead, vncGenericWrite, 
									   vncGenericExecute, vncGenericAll };
	
	// This only does something if we want to use generic access
	// rights, like GENERIC_ALL, in our call to AccessCheck().
	MapGenericMask(&dwAccessDesired, &GenericMapping);
	
	// AccessCheck() requires an impersonation token.
	if ((fn._ImpersonateSecurityContext(phContext) == SEC_E_OK)
		&& OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken)
		&& AccessCheck(psdSD, hToken, dwAccessDesired, &GenericMapping,
		&PrivilegeSet, &dwPrivSetSize, pdwAccessGranted, &fAccessGranted)) {
		// Restrict access to relevant rights only
		fAccessGranted = AreAnyAccessesGranted(*pdwAccessGranted, ViewOnly | Interact);
	}
	
	// End impersonation
	fn._RevertSecurityContext(phContext);
	
	// Close handles
	if (hToken)
		CloseHandle(hToken);
	
	return fAccessGranted;
}

// SplitString splits a string 'input' on the first occurence of char 'separator'
// into string 'head' and 'tail' (removing the separator).
// If separator is not found, head = "" and tail = input.
const char *SplitString(const char *input, char separator, char *head){
	const char *tail;
	int l;

	tail = strchr(input, separator);
	if (tail){
		l = tail - input;
		// get rid of separator
		tail = tail + 1; 
		strncpy(head, input, l);
		head[l] = '\0';
	} else {
		tail   = input;
		head[0] = '\0';
	}
	return tail;
}

bool IsImpersonationAllowed() {
	bool isImpersonationAllowed = false;
	HANDLE hToken = NULL;
	DWORD dwReturnLength = 0;
	LUID impersonatePrivilege;
	TOKEN_PRIVILEGES *ptp = NULL;

	if (!LookupPrivilegeValue(NULL, _T("SeImpersonatePrivilege"), &impersonatePrivilege)
		&& GetLastError() == ERROR_NO_SUCH_PRIVILEGE) {
		// Assume that SeImpersonatePrivilege is not implemented with this OS/SP.
		isImpersonationAllowed = true;
	} else {
		if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)
			&& !GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwReturnLength)
			&& GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
			ptp = (TOKEN_PRIVILEGES *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwReturnLength);
			if (GetTokenInformation(hToken, TokenPrivileges, ptp, dwReturnLength, &dwReturnLength)) {
				for (unsigned int i = 0; i < ptp->PrivilegeCount; i++) {
					// Luid.LowPart/HighPart is ugly. To be improved/changed.
					if ((ptp->Privileges[i].Luid.LowPart == impersonatePrivilege.LowPart)
						&& (ptp->Privileges[i].Luid.HighPart == impersonatePrivilege.HighPart)
						&& (ptp->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED))
						isImpersonationAllowed = true;
				}
			}
		}
	}
	// Close handles
	if (hToken)
		CloseHandle(hToken);
	if (ptp)
		HeapFree(GetProcessHeap(), 0, ptp);

	return isImpersonationAllowed;
}

bool QualifyName(const TCHAR *user, LPTSTR DomName) {
	PSID pSid = NULL;
	DWORD cbSid = 0;
	DWORD cbDomName = MAXLEN;
	SID_NAME_USE sidUse;
	bool isNameQualified = false;

	__try {
		// Get Sid buffer size
		LookupAccountName(NULL, user, pSid, &cbSid, DomName, &cbDomName, &sidUse);
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			__leave;
		if (!(pSid = (PSID) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbSid)))
			__leave;
		if (cbDomName > MAXLEN)
			__leave;
		// Get DomName
		if (!LookupAccountName(NULL, user, pSid, &cbSid, DomName, &cbDomName, &sidUse))
			__leave;
		isNameQualified = true;
	} __finally {
		HeapFree(GetProcessHeap(), 0, pSid);
	}

	return isNameQualified;
}

bool isNT4() {
	OSVERSIONINFO VerInfo;

	VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	if (!GetVersionEx (&VerInfo))   // If this fails, something has gone wrong
		return false;
	
	if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
		VerInfo.dwMajorVersion == 4 &&
		VerInfo.dwMinorVersion == 0)
		return true;
	else
		return false;
}
