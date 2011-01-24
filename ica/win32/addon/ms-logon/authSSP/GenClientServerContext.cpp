/////////////////////////////////////////////////////////////////////////////
//	Copyright (C) 2004 Martin Scharpf, B. Braun Melsungen AG. All Rights Reserved.
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

#include "GenClientServerContext.h"
extern Fn fn;

#define __try
#define __leave goto cleanup
#define __finally cleanup:

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 ) {
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

void UnloadSecurityDll(HMODULE hModule) {

   if (hModule)
      FreeLibrary(hModule);

   fn._AcceptSecurityContext      = NULL;
   fn._AcquireCredentialsHandle   = NULL;
   fn._CompleteAuthToken          = NULL;
   fn._DeleteSecurityContext      = NULL;
   fn._FreeContextBuffer          = NULL;
   fn._FreeCredentialsHandle      = NULL;
   fn._InitializeSecurityContext  = NULL;
   fn._QuerySecurityPackageInfo   = NULL;
   fn._ImpersonateSecurityContext = NULL;
   fn._RevertSecurityContext	  = NULL;
}

HMODULE LoadSecurityDll() {

    HMODULE hModule;
    BOOL    fAllFunctionsLoaded = FALSE; 
	TCHAR   lpszDLL[MAXLEN];
	OSVERSIONINFO VerInfo;

	fn._AcceptSecurityContext      = NULL;
	fn._AcquireCredentialsHandle   = NULL;
	fn._CompleteAuthToken          = NULL;
	fn._DeleteSecurityContext      = NULL;
	fn._FreeContextBuffer          = NULL;
	fn._FreeCredentialsHandle      = NULL;
	fn._InitializeSecurityContext  = NULL;
	fn._QuerySecurityPackageInfo   = NULL;
	fn._ImpersonateSecurityContext = NULL;
	fn._RevertSecurityContext	   = NULL;

	// 
	//  Find out which security DLL to use, depending on
	//  whether we are on NT or Win95 or 2000 or XP or .NET Server
	//  We have to use security.dll on Windows NT 4.0.
	//  All other operating systems, we have to use Secur32.dll
	// 
	VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	if (!GetVersionEx (&VerInfo))   // If this fails, something has gone wrong
		return FALSE;

	if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
		VerInfo.dwMajorVersion == 4 &&
		VerInfo.dwMinorVersion == 0){
			lstrcpy (lpszDLL, _T("security.dll"));
		} else {
			lstrcpy (lpszDLL, _T("secur32.dll"));
		}

	hModule = LoadLibrary(lpszDLL);
	if (!hModule)
		return NULL;

   __try {

      fn._AcceptSecurityContext = (ACCEPT_SECURITY_CONTEXT_FN) 
            GetProcAddress(hModule, "AcceptSecurityContext");
      if (!fn._AcceptSecurityContext)
         __leave;

#ifdef UNICODE
      fn._AcquireCredentialsHandle = (ACQUIRE_CREDENTIALS_HANDLE_FN)
            GetProcAddress(hModule, "AcquireCredentialsHandleW");
#else
      fn._AcquireCredentialsHandle = (ACQUIRE_CREDENTIALS_HANDLE_FN)
            GetProcAddress(hModule, "AcquireCredentialsHandleA");
#endif
      if (!fn._AcquireCredentialsHandle)
         __leave;

      // CompleteAuthToken, ImpersonateSecurityContext and RevertSecurityContext
	  // are not present on Windows 9x Secur32.dll
      // Do not check for the availablity of the functions if it is NULL;
      fn._CompleteAuthToken = (COMPLETE_AUTH_TOKEN_FN) 
            GetProcAddress(hModule, "CompleteAuthToken");

      fn._ImpersonateSecurityContext = (IMPERSONATE_SECURITY_CONTEXT_FN) 
            GetProcAddress(hModule, "ImpersonateSecurityContext");

      fn._RevertSecurityContext = (REVERT_SECURITY_CONTEXT_FN) 
            GetProcAddress(hModule, "RevertSecurityContext");

      fn._DeleteSecurityContext = (DELETE_SECURITY_CONTEXT_FN) 
            GetProcAddress(hModule, "DeleteSecurityContext");
      if (!fn._DeleteSecurityContext)
         __leave;

      fn._FreeContextBuffer = (FREE_CONTEXT_BUFFER_FN) 
            GetProcAddress(hModule, "FreeContextBuffer");
      if (!fn._FreeContextBuffer)
         __leave;

      fn._FreeCredentialsHandle = (FREE_CREDENTIALS_HANDLE_FN) 
            GetProcAddress(hModule, "FreeCredentialsHandle");
      if (!fn._FreeCredentialsHandle)
         __leave;

#ifdef UNICODE
      fn._InitializeSecurityContext = (INITIALIZE_SECURITY_CONTEXT_FN)
            GetProcAddress(hModule, "InitializeSecurityContextW");
#else
      fn._InitializeSecurityContext = (INITIALIZE_SECURITY_CONTEXT_FN) 
            GetProcAddress(hModule, "InitializeSecurityContextA");
#endif
      if (!fn._InitializeSecurityContext)
         __leave;

#ifdef UNICODE
      fn._QuerySecurityPackageInfo = (QUERY_SECURITY_PACKAGE_INFO_FN)
            GetProcAddress(hModule, "QuerySecurityPackageInfoW");
#else
      fn._QuerySecurityPackageInfo = (QUERY_SECURITY_PACKAGE_INFO_FN)
            GetProcAddress(hModule, "QuerySecurityPackageInfoA");
#endif
      if (!fn._QuerySecurityPackageInfo)
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
      ss = fn._AcquireCredentialsHandle(NULL, (SEC_CHAR *) _T("NTLM"), 
            SECPKG_CRED_OUTBOUND, NULL, pAuthIdentity, NULL, NULL,
            &pAS->hcred, &tsExpiry);
      if (ss < 0) {
         fprintf(stderr, "AcquireCredentialsHandle failed with %08X\n", (int) ss);
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

   ss = fn._InitializeSecurityContext(&pAS->hcred, 
         pAS->fInitialized ? &pAS->hctxt : NULL, NULL, 0, 0, 
         SECURITY_NATIVE_DREP, pAS->fInitialized ? &sbdIn : NULL,
         0, &pAS->hctxt, &sbdOut, &fContextAttr, &tsExpiry);
   if (ss < 0)  { 
      // Todo: Better error reporting.
      return FALSE;
   }

   pAS->fHaveCtxtHandle = TRUE;

   // If necessary, complete token
   if (ss == SEC_I_COMPLETE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE) {

      if (fn._CompleteAuthToken) {
         ss = fn._CompleteAuthToken(&pAS->hctxt, &sbdOut);
         if (ss < 0)  {
            fprintf(stderr, "CompleteAuthToken failed with %08X\n", (int) ss);
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
      
      ss = fn._AcquireCredentialsHandle(NULL, (SEC_CHAR *) _T("NTLM"), 
            SECPKG_CRED_INBOUND, NULL, NULL, NULL, NULL, &pAS->hcred, 
            &tsExpiry);
      if (ss < 0) {
         fprintf(stderr, "AcquireCredentialsHandle failed with %08X\n", (int) ss);
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

   ss = fn._AcceptSecurityContext(&pAS->hcred, 
         pAS->fInitialized ? &pAS->hctxt : NULL, &sbdIn, 0, 
         SECURITY_NATIVE_DREP, &pAS->hctxt, &sbdOut, &fContextAttr, 
         &tsExpiry);
   if (ss < 0)  {
      fprintf(stderr, "AcceptSecurityContext failed with %08X\n", (int) ss);
      return FALSE;
   }

   pAS->fHaveCtxtHandle = TRUE;

   // If necessary, complete token
   if (ss == SEC_I_COMPLETE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE) {
      
      if (fn._CompleteAuthToken) {
         ss = fn._CompleteAuthToken(&pAS->hctxt, &sbdOut);
         if (ss < 0)  {
            fprintf(stderr, "CompleteAuthToken failed with %08X\n", (int) ss);
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

