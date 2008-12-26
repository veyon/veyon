/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004 Martin Scharpf, B. Braun Melsungen AG. All Rights Reserved.
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

typedef struct _AUTH_SEQ {
   BOOL fInitialized;
   BOOL fHaveCredHandle;
   BOOL fHaveCtxtHandle;
   CredHandle hcred;
   struct _SecHandle hctxt;
} AUTH_SEQ, *PAUTH_SEQ;

typedef struct _Fn {
ACCEPT_SECURITY_CONTEXT_FN       _AcceptSecurityContext;
ACQUIRE_CREDENTIALS_HANDLE_FN    _AcquireCredentialsHandle;
COMPLETE_AUTH_TOKEN_FN           _CompleteAuthToken;
INITIALIZE_SECURITY_CONTEXT_FN   _InitializeSecurityContext;
DELETE_SECURITY_CONTEXT_FN       _DeleteSecurityContext;
FREE_CONTEXT_BUFFER_FN           _FreeContextBuffer;
FREE_CREDENTIALS_HANDLE_FN       _FreeCredentialsHandle;
QUERY_SECURITY_PACKAGE_INFO_FN   _QuerySecurityPackageInfo;
IMPERSONATE_SECURITY_CONTEXT_FN  _ImpersonateSecurityContext;
REVERT_SECURITY_CONTEXT_FN       _RevertSecurityContext;
} Fn;

