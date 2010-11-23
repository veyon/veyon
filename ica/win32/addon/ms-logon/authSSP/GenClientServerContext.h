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

#include <windows.h>
#include <stdio.h>
#include <lmcons.h>
#include <stdlib.h>
#include <wchar.h>
#include <tchar.h>
#include <lm.h>
#include <stdio.h>
#define SECURITY_WIN32
#include <security.h>
#include <sspi.h>
#ifndef SEC_I_COMPLETE_NEEDED
#include <issperr.h>
#include <time.h>
#endif
#include <aclapi.h>
#pragma hdrstop

#define MAXLEN 256
#define MAX_PREFERRED_LENGTH    ((DWORD) -1)
#define BUFSIZE 1024

typedef DWORD (__stdcall *NetApiBufferFree_t)( void *buf );

typedef DWORD (__stdcall *NetWkstaGetInfoNT_t)( wchar_t *server, DWORD level, byte **buf );
typedef DWORD (__stdcall *NetWkstaGetInfo95_t)( char *domain,DWORD level, byte **buf );

#include "Auth_Seq.h"

BOOL GenClientContext(PAUTH_SEQ pAS, 
					  PSEC_WINNT_AUTH_IDENTITY pAuthIdentity,
					  PVOID pIn, 
					  DWORD cbIn, 
					  PVOID pOut, 
					  PDWORD pcbOut, 
					  PBOOL pfDone);

BOOL GenServerContext(PAUTH_SEQ pAS, 
					  PVOID pIn, 
					  DWORD cbIn, 
					  PVOID pOut,
					  PDWORD pcbOut, 
					  PBOOL pfDone);

void UnloadSecurityDll(HMODULE hModule);

HMODULE LoadSecurityDll();
