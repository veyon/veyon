/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 UltraVNC Team Members. All Rights Reserved.
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
// http://www.uvnc.com
// /macine-vnc Greg Wood (wood@agressiv.com)
#include "workgrpdomnt4.h"

/////////////////////////

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////// 


void UnloadSecurityDll(HMODULE hModule) {

   if (hModule)
      FreeLibrary(hModule);

   _AcceptSecurityContext      = NULL;
   _AcquireCredentialsHandle   = NULL;
   _CompleteAuthToken          = NULL;
   _DeleteSecurityContext      = NULL;
   _FreeContextBuffer          = NULL;
   _FreeCredentialsHandle      = NULL;
   _InitializeSecurityContext  = NULL;
   _QuerySecurityPackageInfo   = NULL;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


HMODULE LoadSecurityDll() {

    HMODULE hModule;
    BOOL    fAllFunctionsLoaded = FALSE; 
	TCHAR   lpszDLL[MAX_PATH];
	OSVERSIONINFO VerInfo;

   _AcceptSecurityContext      = NULL;
   _AcquireCredentialsHandle   = NULL;
   _CompleteAuthToken          = NULL;
   _DeleteSecurityContext      = NULL;
   _FreeContextBuffer          = NULL;
   _FreeCredentialsHandle      = NULL;
   _InitializeSecurityContext  = NULL;
   _QuerySecurityPackageInfo   = NULL;

   // 
   //  Find out which security DLL to use, depending on
   //  whether we are on NT or Win95 or 2000 or XP or .NET Server
   //  We have to use security.dll on Windows NT 4.0.
   //  All other operating systems, we have to use Secur32.dll
   // 
   VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
   if (!GetVersionEx (&VerInfo))   // If this fails, something has gone wrong
   {
      return FALSE;
   }

   if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
      VerInfo.dwMajorVersion == 4 &&
      VerInfo.dwMinorVersion == 0)
   {
      lstrcpy (lpszDLL, _T("security.dll"));
   }
   else
   {
      lstrcpy (lpszDLL, _T("secur32.dll"));
   }


   hModule = LoadLibrary(lpszDLL);
   if (!hModule)
      return NULL;

   __try {

      _AcceptSecurityContext = (ACCEPT_SECURITY_CONTEXT_FN) 
            GetProcAddress(hModule, "AcceptSecurityContext");
      if (!_AcceptSecurityContext)
         __leave;

#ifdef UNICODE
      _AcquireCredentialsHandle = (ACQUIRE_CREDENTIALS_HANDLE_FN)
            GetProcAddress(hModule, "AcquireCredentialsHandleW");
#else
      _AcquireCredentialsHandle = (ACQUIRE_CREDENTIALS_HANDLE_FN)
            GetProcAddress(hModule, "AcquireCredentialsHandleA");
#endif
      if (!_AcquireCredentialsHandle)
         __leave;

      // CompleteAuthToken is not present on Windows 9x Secur32.dll
      // Do not check for the availablity of the function if it is NULL;
      _CompleteAuthToken = (COMPLETE_AUTH_TOKEN_FN) 
            GetProcAddress(hModule, "CompleteAuthToken");

      _DeleteSecurityContext = (DELETE_SECURITY_CONTEXT_FN) 
            GetProcAddress(hModule, "DeleteSecurityContext");
      if (!_DeleteSecurityContext)
         __leave;

      _FreeContextBuffer = (FREE_CONTEXT_BUFFER_FN) 
            GetProcAddress(hModule, "FreeContextBuffer");
      if (!_FreeContextBuffer)
         __leave;

      _FreeCredentialsHandle = (FREE_CREDENTIALS_HANDLE_FN) 
            GetProcAddress(hModule, "FreeCredentialsHandle");
      if (!_FreeCredentialsHandle)
         __leave;

#ifdef UNICODE
      _InitializeSecurityContext = (INITIALIZE_SECURITY_CONTEXT_FN)
            GetProcAddress(hModule, "InitializeSecurityContextW");
#else
      _InitializeSecurityContext = (INITIALIZE_SECURITY_CONTEXT_FN) 
            GetProcAddress(hModule, "InitializeSecurityContextA");
#endif
      if (!_InitializeSecurityContext)
         __leave;

#ifdef UNICODE
      _QuerySecurityPackageInfo = (QUERY_SECURITY_PACKAGE_INFO_FN)
            GetProcAddress(hModule, "QuerySecurityPackageInfoW");
#else
      _QuerySecurityPackageInfo = (QUERY_SECURITY_PACKAGE_INFO_FN)
            GetProcAddress(hModule, "QuerySecurityPackageInfoA");
#endif
      if (!_QuerySecurityPackageInfo)
         __leave;

      fAllFunctionsLoaded = TRUE;

   } __finally {

      if (!fAllFunctionsLoaded) {
         UnloadSecurityDll(hModule);
         hModule = NULL;
      }

   }
   
   return hModule;
}


/////////////////////////////////////////////////////////////////////////////// 


BOOL GenClientContext(PAUTH_SEQ pAS, PSEC_WINNT_AUTH_IDENTITY pAuthIdentity,
      PVOID pIn, DWORD cbIn, PVOID pOut, PDWORD pcbOut, PBOOL pfDone) {

/*++

 Routine Description:

   Optionally takes an input buffer coming from the server and returns
   a buffer of information to send back to the server.  Also returns
   an indication of whether or not the context is complete.

 Return Value:

   Returns TRUE if successful; otherwise FALSE.

--*/ 

   SECURITY_STATUS ss;
   TimeStamp       tsExpiry;
   SecBufferDesc   sbdOut;
   SecBuffer       sbOut;
   SecBufferDesc   sbdIn;
   SecBuffer       sbIn;
   ULONG           fContextAttr;

   if (!pAS->fInitialized) {
      
      ss = _AcquireCredentialsHandle(NULL, _T("NTLM"), 
            SECPKG_CRED_OUTBOUND, NULL, pAuthIdentity, NULL, NULL,
            &pAS->hcred, &tsExpiry);
      if (ss < 0) {
         fprintf(stderr, "AcquireCredentialsHandle failed with %08X\n", ss);
         return FALSE;
      }

      pAS->fHaveCredHandle = TRUE;
   }

   // Prepare output buffer
   sbdOut.ulVersion = 0;
   sbdOut.cBuffers = 1;
   sbdOut.pBuffers = &sbOut;

   sbOut.cbBuffer = *pcbOut;
   sbOut.BufferType = SECBUFFER_TOKEN;
   sbOut.pvBuffer = pOut;

   // Prepare input buffer
   if (pAS->fInitialized)  {
      sbdIn.ulVersion = 0;
      sbdIn.cBuffers = 1;
      sbdIn.pBuffers = &sbIn;

      sbIn.cbBuffer = cbIn;
      sbIn.BufferType = SECBUFFER_TOKEN;
      sbIn.pvBuffer = pIn;
   }

   ss = _InitializeSecurityContext(&pAS->hcred, 
         pAS->fInitialized ? &pAS->hctxt : NULL, NULL, 0, 0, 
         SECURITY_NATIVE_DREP, pAS->fInitialized ? &sbdIn : NULL,
         0, &pAS->hctxt, &sbdOut, &fContextAttr, &tsExpiry);
   if (ss < 0)  { 
      // <winerror.h>
      fprintf(stderr, "InitializeSecurityContext failed with %08X\n", ss);
      return FALSE;
   }

   pAS->fHaveCtxtHandle = TRUE;

   // If necessary, complete token
   if (ss == SEC_I_COMPLETE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE) {

      if (_CompleteAuthToken) {
         ss = _CompleteAuthToken(&pAS->hctxt, &sbdOut);
         if (ss < 0)  {
            fprintf(stderr, "CompleteAuthToken failed with %08X\n", ss);
            return FALSE;
         }
      }
      else {
         fprintf (stderr, "CompleteAuthToken not supported.\n");
         return FALSE;
      }
   }

   *pcbOut = sbOut.cbBuffer;

   if (!pAS->fInitialized)
      pAS->fInitialized = TRUE;

   *pfDone = !(ss == SEC_I_CONTINUE_NEEDED 
         || ss == SEC_I_COMPLETE_AND_CONTINUE );

   return TRUE;
}


/////////////////////////////////////////////////////////////////////////////// 


BOOL GenServerContext(PAUTH_SEQ pAS, PVOID pIn, DWORD cbIn, PVOID pOut,
      PDWORD pcbOut, PBOOL pfDone) {

/*++

 Routine Description:

    Takes an input buffer coming from the client and returns a buffer
    to be sent to the client.  Also returns an indication of whether or
    not the context is complete.

 Return Value:

    Returns TRUE if successful; otherwise FALSE.

--*/ 

   SECURITY_STATUS ss;
   TimeStamp       tsExpiry;
   SecBufferDesc   sbdOut;
   SecBuffer       sbOut;
   SecBufferDesc   sbdIn;
   SecBuffer       sbIn;
   ULONG           fContextAttr;

   if (!pAS->fInitialized)  {
      
      ss = _AcquireCredentialsHandle(NULL, _T("NTLM"), 
            SECPKG_CRED_INBOUND, NULL, NULL, NULL, NULL, &pAS->hcred, 
            &tsExpiry);
      if (ss < 0) {
         fprintf(stderr, "AcquireCredentialsHandle failed with %08X\n", ss);
         return FALSE;
      }

      pAS->fHaveCredHandle = TRUE;
   }

   // Prepare output buffer
   sbdOut.ulVersion = 0;
   sbdOut.cBuffers = 1;
   sbdOut.pBuffers = &sbOut;

   sbOut.cbBuffer = *pcbOut;
   sbOut.BufferType = SECBUFFER_TOKEN;
   sbOut.pvBuffer = pOut;

   // Prepare input buffer
   sbdIn.ulVersion = 0;
   sbdIn.cBuffers = 1;
   sbdIn.pBuffers = &sbIn;

   sbIn.cbBuffer = cbIn;
   sbIn.BufferType = SECBUFFER_TOKEN;
   sbIn.pvBuffer = pIn;

   ss = _AcceptSecurityContext(&pAS->hcred, 
         pAS->fInitialized ? &pAS->hctxt : NULL, &sbdIn, 0, 
         SECURITY_NATIVE_DREP, &pAS->hctxt, &sbdOut, &fContextAttr, 
         &tsExpiry);
   if (ss < 0)  {
      fprintf(stderr, "AcceptSecurityContext failed with %08X\n", ss);
      return FALSE;
   }

   pAS->fHaveCtxtHandle = TRUE;

   // If necessary, complete token
   if (ss == SEC_I_COMPLETE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE) {
      
      if (_CompleteAuthToken) {
         ss = _CompleteAuthToken(&pAS->hctxt, &sbdOut);
         if (ss < 0)  {
            fprintf(stderr, "CompleteAuthToken failed with %08X\n", ss);
            return FALSE;
         }
      }
      else {
         fprintf (stderr, "CompleteAuthToken not supported.\n");
         return FALSE;
      }
   }

   *pcbOut = sbOut.cbBuffer;

   if (!pAS->fInitialized)
      pAS->fInitialized = TRUE;

   *pfDone = !(ss = SEC_I_CONTINUE_NEEDED 
         || ss == SEC_I_COMPLETE_AND_CONTINUE);

   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////// 

BOOL WINAPI SSPLogonUser(LPTSTR szDomain, LPTSTR szUser, LPTSTR szPassword) {

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
   __try {

      hModule = LoadSecurityDll();
      if (!hModule)
         __leave;

      // Get max token size
      _QuerySecurityPackageInfo(_T("NTLM"), &pSPI);
      cbMaxToken = pSPI->cbMaxToken;
      _FreeContextBuffer(pSPI);

      // Allocate buffers for client and server messages
      pClientBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbMaxToken);
      pServerBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbMaxToken);

      // Initialize auth identity structure
      ZeroMemory(&ai, sizeof(ai));
#if defined(UNICODE) || defined(_UNICODE)
      ai.Domain = szDomain;
      ai.DomainLength = lstrlen(szDomain);
      ai.User = szUser;
      ai.UserLength = lstrlen(szUser);
      ai.Password = szPassword;
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

      fResult = TRUE;

   } __finally {

      // Clean up resources
      if (asClient.fHaveCtxtHandle)
         _DeleteSecurityContext(&asClient.hctxt);

      if (asClient.fHaveCredHandle)
         _FreeCredentialsHandle(&asClient.hcred);

      if (asServer.fHaveCtxtHandle)
         _DeleteSecurityContext(&asServer.hctxt);

      if (asServer.fHaveCredHandle)
         _FreeCredentialsHandle(&asServer.hcred);

      if (hModule)
         UnloadSecurityDll(hModule);

      HeapFree(GetProcessHeap(), 0, pClientBuf);
      HeapFree(GetProcessHeap(), 0, pServerBuf);

   }

   return fResult;
}

/////////////////////////////////////////////////////////////////////////////// 
WORKGRPDOMNT4_API
BOOL CUGP(char * userin,char *password,char *machine, char *groupin, int locdom)
{             
//	FILE *file;
	bool isNT = true;
	bool isXP =false;
	bool access_vnc=FALSE;
	bool laccess_vnc=FALSE;
	bool localdatabase=false;
	ad_access=false;
	OSVERSIONINFO ovi = { sizeof ovi };
	// determine OS and load appropriate library

	GetVersionEx( &ovi );
	if ( ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
		isNT = false;
	if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
      ovi.dwMajorVersion == 5 &&
      ovi.dwMinorVersion >= 1) isXP=true;

	HINSTANCE hNet = 0, hLoc = 0;

	if ( isNT )
		{
		hNet = LoadLibrary( "netapi32.dll" );
		}
	else
		{
		hLoc = LoadLibrary( "rlocal32.dll" );
		hNet = LoadLibrary( "radmin32.dll" );
		}

	if ( hNet == 0 )
	{
		printf( "LoadLibrary( %s ) failed, gle = %lu\n",
			isNT? "netapi32.dll": "radmin32.dll", GetLastError() );
		return false;
	}

	// locate needed functions
	NetApiBufferFree_t NetApiBufferFree = 0;
	NetGetDCNameNT_t NetGetDCNameNT = 0;
	NetGetDCName95_t NetGetDCName95 = 0;
	NetUserGetGroupsNT_t NetUserGetGroupsNT = 0;
	NetUserGetGroupsNT_t2 NetUserGetGroupsNT2 = 0;
	NetUserGetGroups95_t NetUserGetGroups95 = 0;
	NetWkstaGetInfoNT_t NetWkstaGetInfoNT = 0;
	NetWkstaGetInfo95_t NetWkstaGetInfo95 = 0;

	NetApiBufferFree = (NetApiBufferFree_t) GetProcAddress( hNet, "NetApiBufferFree" );
	if ( NetApiBufferFree == 0 )
	{
		printf( "Oops! GetProcAddress() failed\n" );
		return false;
	}
	
	if ( isNT )
	{
		NetGetDCNameNT = (NetGetDCNameNT_t) GetProcAddress( hNet, "NetGetDCName" );
		NetUserGetGroupsNT = (NetUserGetGroupsNT_t) GetProcAddress( hNet, "NetUserGetLocalGroups" );
		NetUserGetGroupsNT2 = (NetUserGetGroupsNT_t2) GetProcAddress( hNet, "NetUserGetGroups" );
		NetWkstaGetInfoNT = (NetWkstaGetInfoNT_t)GetProcAddress( hNet,"NetWkstaGetInfo" );
		if ( NetGetDCNameNT == 0 || NetUserGetGroupsNT == 0 || NetWkstaGetInfoNT==0 ||NetUserGetGroupsNT2 == 0)
		{
			printf( "Oops! Some functions not found ...\n" );
			return false;
		}
	}
	else
	{
		NetGetDCName95 = (NetGetDCName95_t) GetProcAddress( hLoc, "LocalNetGetDCName" );
		NetUserGetGroups95 = (NetUserGetGroups95_t) GetProcAddress( hNet, "NetUserGetGroupsA" );
		NetWkstaGetInfo95 = (NetWkstaGetInfo95_t)GetProcAddress( hNet,"NetWkstaGetInfoA" );

		if ( NetGetDCName95 == 0 || NetUserGetGroups95 == 0 ||NetWkstaGetInfo95 == 0)
		{
			printf( "Oops! Some functions not found ...\n" );
			return false;
		}
	}
	// find PDC if necessary; else set up server name for call


	byte *buf = 0;
	DWORD rc,rc2,read, total,i,rcdomain;
	char server[MAXLEN * sizeof(wchar_t)];
	char user[MAXLEN * sizeof(wchar_t)];
	char group[MAXLEN * sizeof(wchar_t)];
	char groupname[MAXLEN];
	char domain[MAXLEN * sizeof(wchar_t)];

	
	if ( isNT )	mbstowcs( (wchar_t *) user, userin, MAXLEN );
	else strcpy( user, userin );

	if ( isNT )	mbstowcs( (wchar_t *) group, groupin, MAXLEN );
	else strcpy( group, groupin );
	////////////////////////////////////////////////////////////////////////////////////////////
	////NT
	////////////////////////////////////////////////////////////////////////////////////////////
if ( isNT )
		{

			printf("Client NT4 or later \n");
			///////////////// search DC ///////////////////////////////////
			rcdomain = NetGetDCNameNT( 0, 0, &buf );
			if (rcdomain==2453) printf( "No domain controler found\n");
			if (!rcdomain)
			{
			printf("Domain controler found");
			wcscpy( (wchar_t *) server, (wchar_t *) buf );
			wprintf((wchar_t *)server);
			printf("\n------------------------------\n");
			}
			printf("Checking Local groups for ");
			printf("user: %S\n", _wcsupr((wchar_t *)user));
			printf("group: %S\n", _wcsupr((wchar_t *)group));

			//////////////////////////////////////////////////////////////////
			///////////////// Local groups ///////////////////////////////////
			//////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////////////////
			if (locdom==1 || locdom==3){
				LOCALGROUP_MEMBERS_INFO_2 *bufLMI, *cur;
				DWORD read, total, resumeh, rc, i;
				wchar_t	tempbuf[MAXLEN];
				wchar_t seperator[MAXLEN];      
				
				netlocalgroupgetmembers_t netlocalgroupgetmembers;
				HINSTANCE h = LoadLibrary ("netapi32.dll");
				netlocalgroupgetmembers = (netlocalgroupgetmembers_t) GetProcAddress (h, "NetLocalGroupGetMembers");

				////////////////////// GROUPS///////////////////////////////////////
				mbstowcs( seperator,  "\\", 5 );
				///////////////////////GROUP1/////////////////////////////////////
				resumeh = 0;
				do
				{
					bufLMI = NULL;
					rc = netlocalgroupgetmembers( NULL, (wchar_t *)group, 2, (BYTE **) &bufLMI,BUFSIZE, &read, &total, &resumeh );
					if ( rc == ERROR_MORE_DATA || rc == ERROR_SUCCESS )
						{
							
							for ( i = 0, cur = bufLMI; i < read; ++ i, ++ cur )
								{
									// Note: the capital S in the format string will expect Unicode
									// strings, as this is a program written/compiled for ANSI.
									_wcsupr(cur->lgrmi2_domainandname);
									wcscpy(tempbuf,cur->lgrmi2_domainandname);
									wchar_t *t=wcsstr(tempbuf,seperator);
									t++;
									printf( "%S\n", t );
									if (wcscmp(_wcsupr(t), _wcsupr((wchar_t *)user))==0) 
										{
											laccess_vnc=TRUE;
											printf( "Local: User found in group \n" );
										}
								}
					if ( bufLMI != NULL )
					NetApiBufferFree( bufLMI );
						}
				} while ( rc == ERROR_MORE_DATA );
				
				///////////////////////////////////////////////////////////////////////
			if ( h != 0 )FreeLibrary( h);
			}


			if ( !rcdomain){
				DWORD rc;
				printf( "New added ----  just for testing \n");
				wcscpy( (wchar_t *) server, (wchar_t *) buf );
				byte *buf2 = 0;
				rc2 = NetWkstaGetInfoNT( 0 , 100 , &buf2 ) ;
				if( rc2 ) printf( "NetWkstaGetInfoA() returned %lu \n", rc2);
				else wcstombs( domain, ((WKSTA_INFO_100_NT *) buf2)->wki100_langroup, MAXLEN );
				NetApiBufferFree( buf2 );
				domain[MAXLEN - 1] = '\0';
				printf("Detected domain = %s\n",domain);
				buf2 = 0;
				char userdomain[MAXLEN * sizeof(wchar_t)];
				char userdom[MAXLEN];
				strcpy(userdom,domain);
				strcat(userdom,"\\");
				strcat(userdom,userin);
				mbstowcs( (wchar_t *) userdomain, userdom, MAXLEN );
				printf( "%S\n", userdomain);
				rc = NetUserGetGroupsNT( NULL ,(wchar_t *) userdomain, 0, 1,&buf2, MAX_PREFERRED_LENGTH, &read, &total);
				if ( rc == NERR_Success)
					{
						for ( i = 0; i < read; ++ i )
							{
								wcstombs( groupname, ((LPLOCALGROUP_USERS_INFO_0_NT *) buf2)[i].grui0_name, MAXLEN );	
								groupname[MAXLEN - 1] = '\0'; // because strncpy won't do this if overflow
#ifdef _MSC_VER
								_strupr(groupname);
								_strupr(groupin);
#else
								_strupr(groupname);
								_strupr(groupin);
#endif
								printf( "compare %s %s\n", groupname, group);
								if (strcmp(groupname, groupin)==0) 
								{
									printf( "match ...\n" );
									laccess_vnc=TRUE;
								}
								else printf( "no match ...\n" );
							}
						if (laccess_vnc==TRUE) printf( "group found ...\n" );
						else printf( "No group found \n" );

					}
				NetApiBufferFree( buf2 );
				
			}
	

			 if (locdom==2 || locdom==3) if ( !rcdomain){
				DWORD rc;
				printf( "New added ----  just for testing \n");
				wcscpy( (wchar_t *) server, (wchar_t *) buf );
				byte *buf2 = 0;
				rc2 = NetWkstaGetInfoNT( 0 , 100 , &buf2 ) ;
				if( rc2 ) printf( "NetWkstaGetInfoA() returned %lu \n", rc2);
				else wcstombs( domain, ((WKSTA_INFO_100_NT *) buf2)->wki100_langroup, MAXLEN );
				NetApiBufferFree( buf2 );
				domain[MAXLEN - 1] = '\0';
				printf("Detected domain = %s\n",domain);
				buf2 = 0;
				char userdomain[MAXLEN * sizeof(wchar_t)];
				char userdom[MAXLEN];
				//strcpy(userdom,domain);
				//strcat(userdom,"\\");
				strcpy(userdom,userin);
				mbstowcs( (wchar_t *) userdomain, userdom, MAXLEN );
				printf( "%S\n", userdomain);
				rc = NetUserGetGroupsNT2( (wchar_t *)server,(wchar_t *) userdomain, 0,&buf2, MAX_PREFERRED_LENGTH, &read, &total);
				if ( rc == NERR_Success)
					{
						for ( i = 0; i < read; ++ i )
							{
								wcstombs( groupname, ((LPLOCALGROUP_USERS_INFO_0_NT *) buf2)[i].grui0_name, MAXLEN );	
								groupname[MAXLEN - 1] = '\0'; // because strncpy won't do this if overflow
#ifdef _MSC_VER
								_strupr(groupname);
								_strupr(groupin);
#else
								_strupr(groupname);
								_strupr(groupin);
#endif
								printf( "compare %s %s\n", groupname, group);
								if (strcmp(groupname, groupin)==0) 
								{
									printf( "match ...\n" );
									laccess_vnc=TRUE;
								}
								else printf( "no match ...\n" );
							}
						if (laccess_vnc==TRUE) printf( "group found ...\n" );
						else printf( "No group found \n" );

					}
				NetApiBufferFree( buf2 );
				
			}
		}
	////////////////////////////////////////////////////////////////////////////////////////////
	////9.X
	////////////////////////////////////////////////////////////////////////////////////////////
	if (locdom==2 || locdom==3) if ( !isNT )
		{
			rc = NetGetDCName95( 0, &buf );
			if ( rc ) printf( "NetGetDCName95() returned %lu\n", rc );
			if (rc==2453) 
			{
				printf( "No domain controler found\n");
				return false;
			}
			if ( !rc )
			{
				strcpy( server, (char *) buf );
				byte *buf2 = 0;
				rc = NetWkstaGetInfo95( 0 , 100 , &buf2 ) ;
				if( rc ) printf( "NetWkstaGetInfoA() returned %lu \n", rc);
				else  strncpy( domain, ((WKSTA_INFO_100_95 *) buf2)->wki100_langroup, MAXLEN );
				NetApiBufferFree( buf2 );
				domain[MAXLEN - 1] = '\0';
				NetApiBufferFree( buf );
				buf = 0;
				buf2 = 0;
				////////////////////////////////////////////
				rc = NetUserGetGroups95( server,user, 0, &buf2, 2024, &read, &total);
				if ( rc == NERR_Success)
					{
						for ( i = 0; i < read; ++ i )
							{ 
								strncpy( groupname, ((LPLOCALGROUP_USERS_INFO_0_95 *) buf2)[i].grui0_name, MAXLEN );
								groupname[MAXLEN - 1] = '\0'; // because strncpy won't do this if overflow
#ifdef _MSC_VER
							_strupr(groupname);
#else
							_strupr(groupname);
#endif
								printf( "%s\n", groupname);
								if (strcmp(groupname, group)==0) laccess_vnc=TRUE;
							}
						if (access_vnc==TRUE) printf( "NT: Domain group found ...\n" );
						else printf( "No Domain group found \n" );
					}
				NetApiBufferFree( buf2 );
				buf2=0;
			}
		}
		
	
	if (buf)NetApiBufferFree( buf );
	if ( hNet != 0 )
		FreeLibrary( hNet );
	if ( hLoc != 0 )
		FreeLibrary( hLoc );
	//check user
	if (isNT)
	{

#if _MSC_VER < 1400
		wcstombs( user, (unsigned short *)user, MAXLEN);	
#else
		wcstombs( user, (const wchar_t *)user, MAXLEN);	
#endif
	}

	BOOL logon_OK=false;
	BOOL passwd_OK=false;
#ifdef _MSC_VER
	_strupr(user);
#else
	_strupr(user);
#endif
	passwd_OK=false;
	if (isXP)
		{
			LOGONUSER LU=NULL;
			HMODULE  shell1 = LoadLibrary("advapi32.dll");
			if(shell1!=NULL) LU = (LOGONUSER)GetProcAddress((HINSTANCE)shell1,"LogonUserA");
			if (LU!=NULL)
			{
			HANDLE token;
			if (locdom==2 || locdom==3 ||locdom==1) 
				if (LU(user,domain,password,LOGON32_LOGON_INTERACTIVE,LOGON32_PROVIDER_DEFAULT,&token))
				{	passwd_OK=true;
					if (passwd_OK==TRUE) printf( "Domain password check OK \n" );
					else printf( "Domain password check Failed \n" );
				}
			if (locdom==1 || locdom==3) 
				if (LU(user,".",password,LOGON32_LOGON_INTERACTIVE,LOGON32_PROVIDER_DEFAULT,&token))
				{
				passwd_OK=true;
				if (passwd_OK==TRUE) printf( "Local password check OK \n" );
				}
			}
			FreeLibrary(shell1);

		}
	else
	{
		if (locdom==2 || locdom==3 || locdom==1)
		if (SSPLogonUser(domain,user, password))
		{	passwd_OK=true;
			if (passwd_OK==TRUE) printf( "Domain password check OK \n" );		
		}
		else printf( "Domain password check Failed \n" );

		if (locdom==1 || locdom==3)
		if (SSPLogonUser(".",user, password))
		{
			passwd_OK=true;
			if (passwd_OK==TRUE) printf( "Local password check OK \n" );
		}
		else printf( "Local password check Failed \n" );
		if (locdom==2 || locdom==3)if (SSPLogonUser(domain,"isdiua", "hegbfsa")) {passwd_OK=false;printf( "Guest account block \n" );}
		if (locdom==1 || locdom==3)if (SSPLogonUser(".","isdiua", "hegbfsa")) {passwd_OK=false;printf( "Guest account block \n" );}
	}
	if (access_vnc==TRUE || laccess_vnc==TRUE || ad_access==TRUE)
	{
		if (passwd_OK) 
		{
		logon_OK=true;
		printf( "Acces to vnc  OK \n" );
		}
	}
	return logon_OK;
	
}


