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
#include "authadmin.h"
#include "lm.h"

typedef struct _WKSTA_INFO_100_NT {
  DWORD     wki100_platform_id;
  wchar_t *    wki100_computername;
  wchar_t *    wki100_langroup;
  DWORD     wki100_ver_major;
  DWORD     wki100_ver_minor;
}WKSTA_INFO_100_NT;


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


bool IsAdmin() { 
       SC_HANDLE hSC;
       hSC = OpenSCManager(
             NULL,
              NULL,
              GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE
              );
    if( hSC == NULL ) {
                  return FALSE;
    }   
         CloseServiceHandle( hSC );
         return TRUE;
}

AUTHADMIN_API
BOOL CUGP(char * userin,char *password,char *machine,char *groupin,int locdom)
{
	DWORD dwLogonType;
	DWORD dwLogonProvider;
	HANDLE hToken;
	bool returnvalue=false;
	dwLogonType     = LOGON32_LOGON_INTERACTIVE;
	dwLogonProvider = LOGON32_PROVIDER_DEFAULT;

	byte *buf = 0;
	byte *buf2 = 0;
	char domain[MAXLEN * sizeof(wchar_t)];
	DWORD rcdomain = NetGetDCName( 0, 0, &buf );
	NetApiBufferFree( buf );
	printf("Logonuser: % s %s \n", userin, ".");
			if (LogonUser(userin, ".", password, dwLogonType, dwLogonProvider, &hToken))
					if (ImpersonateLoggedOnUser(hToken))
				{
					returnvalue=IsAdmin();
					RevertToSelf();
					CloseHandle(hToken);
				}
	if (returnvalue==true) return returnvalue;
	if (!rcdomain)
		{
			DWORD result=NetWkstaGetInfo( 0 , 100 , &buf2 ) ;
				if (!result)
				{
					wcstombs( domain, ((WKSTA_INFO_100_NT *) buf2)->wki100_langroup, MAXLEN );
					NetApiBufferFree( buf2 );
					printf("Logonuser: % s %s \n", userin, domain);
					if (LogonUser(userin, domain, password, dwLogonType, dwLogonProvider, &hToken))
						if (ImpersonateLoggedOnUser(hToken))
							{
								returnvalue=IsAdmin();
								RevertToSelf();
								CloseHandle(hToken);
							}
				}

		}
	
	return returnvalue;
	
}