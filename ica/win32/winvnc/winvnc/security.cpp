#include "stdhdrs.h"
#include "lmcons.h"
#include "vncservice.h"
#include "vncOSVersion.h"
#include "common/ScopeGuard.h"


typedef BOOL (WINAPI *pWTSQueryUserToken_proto)(ULONG, PHANDLE);

DWORD GetCurrentUserToken(HANDLE& process, HANDLE &Token)
{
	if(OSversion()==4 || OSversion()==5) return 1;
	if (!vncService::RunningAsService())
	{
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &Token))
			return 0;
		return 2;
	}
	{
	  HWND tray = FindWindow(("Shell_TrayWnd"), 0);
      if (!tray)
        return 0;
	  DWORD processId = 0;
      GetWindowThreadProcessId(tray, &processId);
      if (!processId)
        return 0;
	  process = OpenProcess(MAXIMUM_ALLOWED, FALSE, processId);
      if (!process)
        return 0;
	  OpenProcessToken(process, MAXIMUM_ALLOWED, &Token);
	  // Remeber the closhandle
	  //
	}
	return 2;
}

bool ImpersonateCurrentUser() {
  SetLastError(0);
  HANDLE process=0;
  HANDLE Token=NULL;

  GetCurrentUserToken(process, Token);

  // Auto close process and token when leaving scope
  ON_BLOCK_EXIT(CloseHandle, process);
  ON_BLOCK_EXIT(CloseHandle, Token);

  bool test=(FALSE != ImpersonateLoggedOnUser(Token));

  return test;
}

bool
RevertCurrentUser() {
  return (FALSE != RevertToSelf());
}

bool RunningAsAdministrator ()
{
 BOOL   fAdmin;
 TOKEN_GROUPS *ptg = NULL;
 DWORD  cbTokenGroups;
 DWORD  dwGroup;
 PSID   psidAdmin;
 SetLastError(0);
 HANDLE process=0;
 HANDLE Token=NULL;

 if (GetCurrentUserToken(process, Token)==1) return true;

 ON_BLOCK_EXIT(CloseHandle, process);
 ON_BLOCK_EXIT(CloseHandle, Token);

 SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;

 // Then we must query the size of the group information associated with
 // the token. Note that we expect a FALSE result from GetTokenInformation
 // because we've given it a NULL buffer. On exit cbTokenGroups will tell
 // the size of the group information.

 if ( GetTokenInformation ( Token, TokenGroups, NULL, 0, &cbTokenGroups))
  return ( FALSE);

 // Here we verify that GetTokenInformation failed for lack of a large
 // enough buffer.
 DWORD aa=GetLastError();
 if ( aa != ERROR_INSUFFICIENT_BUFFER)
  return ( FALSE);

 // Now we allocate a buffer for the group information.
 // Since _alloca allocates on the stack, we don't have
 // to explicitly deallocate it. That happens automatically
 // when we exit this function.

 if ( ! ( ptg= (_TOKEN_GROUPS *) _malloca ( cbTokenGroups))) 
  return ( FALSE);

 // Now we ask for the group information again.
 // This may fail if an administrator has added this account
 // to an additional group between our first call to
 // GetTokenInformation and this one.

 if ( !GetTokenInformation ( Token, TokenGroups, ptg, cbTokenGroups,
          &cbTokenGroups) )
  return ( FALSE);

 // Now we must create a System Identifier for the Admin group.

 if ( ! AllocateAndInitializeSid ( &SystemSidAuthority, 2, 
            SECURITY_BUILTIN_DOMAIN_RID, 
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &psidAdmin) )
  return ( FALSE);

 // Finally we'll iterate through the list of groups for this access
 // token looking for a match against the SID we created above.

 fAdmin= FALSE;

 for ( dwGroup= 0; dwGroup < ptg->GroupCount; dwGroup++)
 {
  if ( EqualSid ( ptg->Groups[dwGroup].Sid, psidAdmin))
  {
   fAdmin = TRUE;

   break;
  }
 }

 // Before we exit we must explicity deallocate the SID we created.

 FreeSid ( psidAdmin);
 return (FALSE != fAdmin);
}
