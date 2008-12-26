
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the AUTH_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// AUTH_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef WORKGRPDOMNT4_EXPORTS
#define WORKGRPDOMNT4_API __declspec(dllexport)
#else
#define WORKGRPDOMNT4_API __declspec(dllimport)
#endif
#pragma comment( lib, "netapi32.lib" )
#if defined( UNICODE ) || defined( _UNICODE )
#error Sorry -- please compile as an ANSI program.
#endif
#include <windows.h>
#include <stdio.h>
#include <lmcons.h>
#include <stdlib.h>
#include <wchar.h>
#include <tchar.h>
#include <lm.h>
#include <stdio.h>
#define SECURITY_WIN32
#include <sspi.h>
#ifndef SEC_I_COMPLETE_NEEDED
#include <issperr.h>
#include <time.h>
#endif
#pragma hdrstop

#define MAXLEN 256
#define MAX_PREFERRED_LENGTH    ((DWORD) -1)
#define NERR_Success            0
//#define LG_INCLUDE_INDIRECT 1
#define BUFSIZE 1024

typedef DWORD (__stdcall *NetApiBufferFree_t)( void *buf );
typedef DWORD (__stdcall *NetGetDCNameNT_t)( wchar_t *server, wchar_t *domain, byte **buf );
typedef DWORD (__stdcall *NetGetDCName95_t)( char *domain, byte **buf );
typedef DWORD (__stdcall *NetUserGetGroupsNT_t)( wchar_t *server,wchar_t *user, DWORD level,DWORD flags, byte **buf,DWORD prefmaxlen, DWORD *entriesread, DWORD *totalentries);
typedef DWORD (__stdcall *NetUserGetGroupsNT_t2)( wchar_t *server,wchar_t *user, DWORD level, byte **buf,DWORD prefmaxlen, DWORD *entriesread, DWORD *totalentries);

typedef DWORD (__stdcall *NetUserGetGroups95_t)( char *server,char *user, DWORD level, byte **buf,DWORD prefmaxlen, DWORD *entriesread, DWORD *totalentries);
typedef DWORD (__stdcall *NetWkstaGetInfoNT_t)( wchar_t *server, DWORD level, byte **buf );
typedef DWORD (__stdcall *NetWkstaGetInfo95_t)( char *domain,DWORD level, byte **buf );
typedef DWORD (__stdcall *netlocalgroupgetmembers_t)(LPWSTR,LPWSTR,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,PDWORD);

typedef struct _LPLOCALGROUP_USERS_INFO_0_NT
{
	wchar_t *grui0_name;
}LPLOCALGROUP_USERS_INFO_0_NT;
typedef struct _LPLOCALGROUP_USERS_INFO_0_95
{
	char *grui0_name;
}LPLOCALGROUP_USERS_INFO_0_95;
typedef struct _WKSTA_INFO_100_95 {
  DWORD     wki100_platform_id;
  char *    wki100_computername;
  char *    wki100_langroup;
  DWORD     wki100_ver_major;
  DWORD     wki100_ver_minor;
}WKSTA_INFO_100_95;
typedef struct _WKSTA_INFO_100_NT {
  DWORD     wki100_platform_id;
  wchar_t *    wki100_computername;
  wchar_t *    wki100_langroup;
  DWORD     wki100_ver_major;
  DWORD     wki100_ver_minor;
}WKSTA_INFO_100_NT;
typedef struct _AUTH_SEQ {
   BOOL fInitialized;
   BOOL fHaveCredHandle;
   BOOL fHaveCtxtHandle;
   CredHandle hcred;
   struct _SecHandle hctxt;
} AUTH_SEQ, *PAUTH_SEQ;

typedef BOOL (WINAPI *LOGONUSER)(
        LPTSTR lpszUsername,    // user name
        LPTSTR lpszDomain,      // domain or server
        LPTSTR lpszPassword,    // password
        DWORD dwLogonType,      // type of logon operation
        DWORD dwLogonProvider,  // logon provider
        PHANDLE phToken );        // receive tokens handle


ACCEPT_SECURITY_CONTEXT_FN       _AcceptSecurityContext;
ACQUIRE_CREDENTIALS_HANDLE_FN    _AcquireCredentialsHandle;
COMPLETE_AUTH_TOKEN_FN           _CompleteAuthToken;
DELETE_SECURITY_CONTEXT_FN       _DeleteSecurityContext;
FREE_CONTEXT_BUFFER_FN           _FreeContextBuffer;
FREE_CREDENTIALS_HANDLE_FN       _FreeCredentialsHandle;
INITIALIZE_SECURITY_CONTEXT_FN   _InitializeSecurityContext;
QUERY_SECURITY_PACKAGE_INFO_FN   _QuerySecurityPackageInfo;

bool ad_access;
WORKGRPDOMNT4_API BOOL CUGP(char * userin,char *password,char *machine, char *group, int locdom);

