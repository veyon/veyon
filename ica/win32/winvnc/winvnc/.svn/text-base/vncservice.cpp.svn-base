//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
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
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// vncService

// Implementation of service-oriented functionality of WinVNC

#include "stdhdrs.h"

// Header

#include "vncService.h"

#include <lmcons.h>
#include "omnithread.h"
#include "WinVNC.h"
#include "vncMenu.h"
#include "vncTimedMsgBox.h"

#include "localization.h" // Act : add localization on messages


// OS-SPECIFIC ROUTINES

// Create an instance of the vncService class to cause the static fields to be
// initialised properly

vncService init;
bool G_1111=false;
DWORD	g_platform_id;
BOOL	g_impersonating_user = 0;
DWORD	g_version_major;
DWORD	g_version_minor;
BOOL	m_fRunningFromExternalService = false;

typedef DWORD (WINAPI* pWTSGetActiveConsoleSessionId)(VOID);
typedef BOOL (WINAPI * pProcessIdToSessionId)(DWORD,DWORD*);
#include <tlhelp32.h>

pProcessIdToSessionId WTSProcessIdToSessionIdF=NULL;

#ifndef SM_REMOTESESSION
#define SM_REMOTESESSION 0X1000
#endif

void ClearKeyState(BYTE key);
DWORD GetCurrentSessionID()
{
	DWORD dwSessionId;
	pWTSGetActiveConsoleSessionId WTSGetActiveConsoleSessionIdF=NULL;
	WTSProcessIdToSessionIdF=NULL;

	HMODULE  hlibkernel = LoadLibrary("kernel32.dll"); 
	WTSGetActiveConsoleSessionIdF=(pWTSGetActiveConsoleSessionId)GetProcAddress(hlibkernel, "WTSGetActiveConsoleSessionId");
	WTSProcessIdToSessionIdF=(pProcessIdToSessionId)GetProcAddress(hlibkernel, "ProcessIdToSessionId");
	if (WTSGetActiveConsoleSessionIdF!=NULL)
	   dwSessionId =WTSGetActiveConsoleSessionIdF();
	else dwSessionId=0;

	if( GetSystemMetrics( SM_REMOTESESSION))
		if (WTSProcessIdToSessionIdF!=NULL)
		{
			DWORD dw		 = GetCurrentProcessId();
			DWORD pSessionId = 0xFFFFFFFF;
			WTSProcessIdToSessionIdF( dw, &pSessionId );
			dwSessionId=pSessionId;
		}
	FreeLibrary(hlibkernel);
	return dwSessionId;
}

DWORD GetExplorerLogonPid()
{
	DWORD dwSessionId;
	DWORD dwExplorerLogonPid=0;
	PROCESSENTRY32 procEntry;
//	HANDLE hProcess,hPToken;

	pWTSGetActiveConsoleSessionId WTSGetActiveConsoleSessionIdF=NULL;
	WTSProcessIdToSessionIdF=NULL;

	HMODULE  hlibkernel = LoadLibrary("kernel32.dll"); 
	WTSGetActiveConsoleSessionIdF=(pWTSGetActiveConsoleSessionId)GetProcAddress(hlibkernel, "WTSGetActiveConsoleSessionId");
	WTSProcessIdToSessionIdF=(pProcessIdToSessionId)GetProcAddress(hlibkernel, "ProcessIdToSessionId");
	if (WTSGetActiveConsoleSessionIdF!=NULL)
	   dwSessionId =WTSGetActiveConsoleSessionIdF();
	else dwSessionId=0;

	if( GetSystemMetrics( SM_REMOTESESSION))
		if (WTSProcessIdToSessionIdF!=NULL)
		{
			DWORD dw		 = GetCurrentProcessId();
			DWORD pSessionId = 0xFFFFFFFF;
			WTSProcessIdToSessionIdF( dw, &pSessionId );
			dwSessionId=pSessionId;
		}

	

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
    {
		FreeLibrary(hlibkernel);
        return 0 ;
    }

    procEntry.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnap, &procEntry))
    {
		CloseHandle(hSnap);
		FreeLibrary(hlibkernel);
        return 0 ;
    }

    do
    {
        if (_stricmp(procEntry.szExeFile, "explorer.exe") == 0)
        {
          DWORD dwExplorerSessId = 0;
		  if (WTSProcessIdToSessionIdF!=NULL)
		  {
			  if (WTSProcessIdToSessionIdF(procEntry.th32ProcessID, &dwExplorerSessId) 
						&& dwExplorerSessId == dwSessionId)
				{
					dwExplorerLogonPid = procEntry.th32ProcessID;
					break;
				}
		  }
		  else dwExplorerLogonPid = procEntry.th32ProcessID;
        }

    } while (Process32Next(hSnap, &procEntry));
	CloseHandle(hSnap);
	FreeLibrary(hlibkernel);
	return dwExplorerLogonPid;
}

char aa[16384];
#include <tlhelp32.h>
bool
GetConsoleUser(char *buffer, UINT size)
{

	HANDLE hProcess,hPToken;
	DWORD dwExplorerLogonPid=GetExplorerLogonPid();
	if (dwExplorerLogonPid==0) 
	{
		strcpy(buffer,"");
		return 0;
	}
	hProcess = OpenProcess(MAXIMUM_ALLOWED,FALSE,dwExplorerLogonPid);

   if(!::OpenProcessToken(hProcess,TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY
                                    |TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_SESSIONID
                                    |TOKEN_READ|TOKEN_WRITE,&hPToken))
		{     
			   strcpy(buffer,"");
			   CloseHandle(hProcess);
			   return 0 ;
		}


   // token user
    TOKEN_USER *ptu;
	DWORD needed;
	ptu = (TOKEN_USER *) aa;//malloc( 16384 );
	if (GetTokenInformation( hPToken, TokenUser, ptu, 16384, &needed ) )
	{
		char  DomainName[64];
		memset(DomainName, 0, sizeof(DomainName));
		DWORD DomainSize;
		DomainSize =sizeof(DomainName)-1;
		SID_NAME_USE SidType;
		DWORD dwsize=size;
		LookupAccountSid(NULL, ptu->User.Sid, buffer, &dwsize, DomainName, &DomainSize, &SidType);
		//free(ptu);
		CloseHandle(hPToken);
		CloseHandle(hProcess);
		return 1;
	}
	//free(ptu);
	strcpy(buffer,"");
	CloseHandle(hPToken);
	CloseHandle(hProcess);
	return 0;
}



vncService::vncService()
{
    OSVERSIONINFO osversioninfo;
    osversioninfo.dwOSVersionInfoSize = sizeof(osversioninfo);

    // Get the current OS version
    if (!GetVersionEx(&osversioninfo))
	    g_platform_id = 0;
    g_platform_id = osversioninfo.dwPlatformId;
	g_version_major = osversioninfo.dwMajorVersion;
	g_version_minor = osversioninfo.dwMinorVersion;
}

// CurrentUser - fills a buffer with the name of the current user!
BOOL
GetCurrentUser(char *buffer, UINT size) // RealVNC 336 change
{	
	if (vncService::RunningFromExternalService())
	{
//		vnclog.Print(LL_INTERR, VNCLOG("@@@@@@@@@@@@@ GetCurrentUser - Forcing g_impersonating_user \n"));
		g_impersonating_user = TRUE;
	}

	// How to obtain the name of the current user depends upon the OS being used
	if ((g_platform_id == VER_PLATFORM_WIN32_NT) && vncService::RunningAsService())
	{
		// Windows NT, service-mode

		// -=- FIRSTLY - verify that a user is logged on

		// Get the current Window station
		HWINSTA station = GetProcessWindowStation();
		if (station == NULL)
		{
			vnclog.Print(LL_INTERR, VNCLOG("@@@@@@@@@@@@@ GetCurrentUser - ERROR : No window station \n"));
			return FALSE;
		}

		// Get the current user SID size
		DWORD usersize;
		GetUserObjectInformation(station,
			UOI_USER_SID, NULL, 0, &usersize);
		DWORD  dwErrorCode = GetLastError();
		SetLastError(0);

		// Check the required buffer size isn't zero
		if (usersize == 0)
		{
			// No user is logged in - ensure we're not impersonating anyone
			RevertToSelf();
			g_impersonating_user = FALSE;

			// Return "" as the name...
			if (strlen("") >= size)
			{
				vnclog.Print(LL_INTERR, VNCLOG("@@@@@@@@@@@@@ GetCurrentUser - Error: Bad buffer size \n"));
				return FALSE;
			}
			strcpy(buffer, "");

			vnclog.Print(LL_INTERR, VNCLOG("@@@@@@@@@@@@@ GetCurrentUser - Error: Usersize 0\n"));
			return TRUE;
		}

		// -=- SECONDLY - a user is logged on but if we're not impersonating
		//     them then we can't continue!
		if (!g_impersonating_user)
		{
			vnclog.Print(LL_INTERR, VNCLOG("@@@@@@@@@@@@@ GetCurrentUser - Error: NOT impersonating user \n"));
			// Return "" as the name...
			if (strlen("") >= size)
				return FALSE;
			strcpy(buffer, "");
			return TRUE;
		}
	}
		
	// -=- When we reach here, we're either running under Win9x, or we're running
	//     under NT as an application or as a service impersonating a user
	// Either way, we should find a suitable user name.

	switch (g_platform_id)
	{

	case VER_PLATFORM_WIN32_WINDOWS:
	case VER_PLATFORM_WIN32_NT:
		{
			// Just call GetCurrentUser
			DWORD length = size;

//			vnclog.Print(LL_INTERR, VNCLOG("@@@@@@@@@@@@@ GetCurrentUser - GetUserName call \n"));
			if ( GetConsoleUser(buffer, size) == 0)
			{
				if (GetUserName(buffer, &length) == 0)
				{
					UINT error = GetLastError();

					if (error == ERROR_NOT_LOGGED_ON)
					{
						vnclog.Print(LL_INTERR, VNCLOG("@@@@@@@@@@@@@ GetCurrentUser - Error: No user logged on \n"));
						// No user logged on
						if (strlen("") >= size)
							return FALSE;
						strcpy(buffer, "");
						return TRUE;
					}
					else
					{
						// Genuine error...
						vnclog.Print(LL_INTERR, VNCLOG("getusername error %d\n"), GetLastError());
						return FALSE;
					}
				}
			}
		}
		vnclog.Print(LL_INTERR, VNCLOG("@@@@@@@@@@@@@ GetCurrentUser - UserNAme found: %s \n"), buffer);
		return TRUE;
	};

	// OS was not recognised!
	vnclog.Print(LL_INTERR, VNCLOG("@@@@@@@@@@@@@ GetCurrentUser - Error: Unknown OS \n"));
	return FALSE;
}

// RealVNC 336 change
BOOL
vncService::CurrentUser(char *buffer, UINT size)
{
  BOOL result = GetCurrentUser(buffer, size);
  if (result && (strcmp(buffer, "") == 0) && !vncService::RunningAsService()) {
    strncpy(buffer, "Default", size);
  }
  return result;
}


BOOL vncService::IsWSLocked()
{
	if (!IsWinNT()) 
		return false;

	bool bLocked = false;


	HDESK hDesk;
	BOOL bRes;
	DWORD dwLen;
	char sName[200];
	
	hDesk = OpenInputDesktop(0, FALSE, 0);

	if (hDesk == NULL)
	{
		 bLocked = true;
	}
	else 
	{
		bRes = GetUserObjectInformation(hDesk, UOI_NAME, sName, sizeof(sName), &dwLen);

		if (bRes)
			sName[dwLen]='\0';
		else
			sName[0]='\0';

		CloseDesktop(hDesk);

		if (_stricmp(sName,"Default") != 0)
			 bLocked = true; // WS is locked or screen saver active
		else
			 bLocked = false ;
	}

	return bLocked;
}


// IsWin95 - returns a BOOL indicating whether the current OS is Win95
BOOL
vncService::IsWin95()
{
	return (g_platform_id == VER_PLATFORM_WIN32_WINDOWS);
}

// IsWinNT - returns a bool indicating whether the current OS is WinNT
BOOL
vncService::IsWinNT()
{
	return (g_platform_id == VER_PLATFORM_WIN32_NT);
}

// Version info
DWORD
vncService::VersionMajor()
{
	return g_version_major;
}

DWORD
vncService::VersionMinor()
{
	return g_version_minor;
}

// Internal routine to find the WinVNC menu class window and
// post a message to it!

BOOL
PostToWinVNC(UINT message, WPARAM wParam, LPARAM lParam)
{
	// Locate the hidden WinVNC menu window
	HWND hservwnd = FindWindow(MENU_CLASS_NAME, NULL);
	if (hservwnd == NULL)
		return FALSE;

	// Post the message to WinVNC
	PostMessage(hservwnd, message, wParam, lParam);
	return TRUE;
}

// Static routines only used on Windows NT to ensure we're in the right desktop
// These routines are generally available to any thread at any time.

// - SelectDesktop(HDESK)
// Switches the current thread into a different desktop by deskto handle
// This call takes care of all the evil memory management involved

BOOL
vncService::SelectHDESK(HDESK new_desktop)
{
	// Are we running on NT?
	if (IsWinNT())
	{
		HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());

		DWORD dummy;
		char new_name[256];

		if (!GetUserObjectInformation(new_desktop, UOI_NAME, &new_name, 256, &dummy)) {
			vnclog.Print(LL_INTERR, VNCLOG("!GetUserObjectInformation \n"));
			return FALSE;
		}

		vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK to %s (%x) from %x\n"), new_name, new_desktop, old_desktop);

		// Switch the desktop
		if(!SetThreadDesktop(new_desktop)) {
			vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK:!SetThreadDesktop \n"));
			return FALSE;
		}

		return TRUE;
	}

	return TRUE;
}

// - SelectDesktop(char *)
// Switches the current thread into a different desktop, by name
// Calling with a valid desktop name will place the thread in that desktop.
// Calling with a NULL name will place the thread in the current input desktop.
BOOL
vncService::SelectDesktop(char *name)
{
	//return false;
	// Are we running on NT?
	if (IsWinNT())
	{
		HDESK desktop;
		vnclog.Print(LL_INTERR, VNCLOG("SelectDesktop \n"));
		if (name != NULL)
		{
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 named\n"));
			// Attempt to open the named desktop
			desktop = OpenDesktop(name, 0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		}
		else
		{
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 NULL\n"));
			// No, so open the input desktop
			desktop = OpenInputDesktop(0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		}

		// Did we succeed?
		if (desktop == NULL) {
				vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 \n"));
				return FALSE;
		}
		else vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 OK\n"));

		// Switch to the new desktop
		if (!SelectHDESK(desktop)) {
			// Failed to enter the new desktop, so free it!
			if (!CloseDesktop(desktop))
				vnclog.Print(LL_INTERR, VNCLOG("SelectDesktop failed to close desktop\n"));
			return FALSE;
		}

		// We successfully switched desktops!
		return TRUE;
	}

	return (name == NULL);
}



// Find the visible window station and switch to it
// This would allow the service to be started non-interactive
// Needs more supporting code & a redesign of the server core to
// work, with better partitioning between server & UI components.

static HWINSTA home_window_station = GetProcessWindowStation();

BOOL CALLBACK WinStationEnumProc(LPTSTR name, LPARAM param) {
	HWINSTA station = OpenWindowStation(name, FALSE, GENERIC_ALL);
	HWINSTA oldstation = GetProcessWindowStation();
	USEROBJECTFLAGS flags;
	if (!GetUserObjectInformation(station, UOI_FLAGS, &flags, sizeof(flags), NULL)) {
		return TRUE;
	}
	BOOL visible = flags.dwFlags & WSF_VISIBLE;
	if (visible) {
		if (SetProcessWindowStation(station)) {
			if (oldstation != home_window_station) {
				CloseWindowStation(oldstation);
			}
		} else {
			CloseWindowStation(station);
		}
		return FALSE;
	}
	return TRUE;
}

// NT only function to establish whether we're on the current input desktop

BOOL
vncService::InputDesktopSelected()
{
//	vnclog.Print(LL_INTERR, VNCLOG("InputDesktopSelected()\n"));
	// Are we running on NT?
	if (IsWinNT())
	{
		// Get the input and thread desktops
		HDESK threaddesktop = GetThreadDesktop(GetCurrentThreadId());
		HDESK inputdesktop = OpenInputDesktop(0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP);


		// Get the desktop names:
		// *** I think this is horribly inefficient but I'm not sure.
		if (inputdesktop == NULL)
		{
			DWORD lasterror;
			lasterror=GetLastError();
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputDesktop I\n"));
			if (lasterror==170) return TRUE;
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputDesktop II\n"));
			return FALSE;
		}

		DWORD dummy;
		char threadname[256];
		char inputname[256];

		if (!GetUserObjectInformation(threaddesktop, UOI_NAME, &threadname, 256, &dummy)) {
			if (!CloseDesktop(inputdesktop))
				vnclog.Print(LL_INTERR, VNCLOG("failed to close input desktop\n"));
			vnclog.Print(LL_INTERR, VNCLOG("!GetUserObjectInformation(threaddesktop\n"));
			return FALSE;
		}
		_ASSERT(dummy <= 256);
		if (!GetUserObjectInformation(inputdesktop, UOI_NAME, &inputname, 256, &dummy)) {
			if (!CloseDesktop(inputdesktop))
				vnclog.Print(LL_INTERR, VNCLOG("failed to close input desktop\n"));
			vnclog.Print(LL_INTERR, VNCLOG("!GetUserObjectInformation(inputdesktop\n"));
			return FALSE;
		}
		_ASSERT(dummy <= 256);

		if (!CloseDesktop(inputdesktop))
			vnclog.Print(LL_INTERR, VNCLOG("failed to close input desktop\n"));

		if (strcmp(threadname, inputname) != 0)
		{
			vnclog.Print(LL_INTERR, VNCLOG("threadname, inputname differ\n"));
		   return FALSE;
		}	
	}

	return TRUE;
}


// Static routine used to fool Winlogon into thinking CtrlAltDel was pressed

void *
SimulateCtrlAltDelThreadFn(void *context)
{
	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());

	// Switch into the Winlogon desktop
	if (!vncService::SelectDesktop("Winlogon"))
	{
		vnclog.Print(LL_INTERR, VNCLOG("failed to select logon desktop\n"));
		vncTimedMsgBox::Do(
									sz_ID_CADERROR,
									sz_ID_ULTRAVNC_WARNING,
									MB_ICONINFORMATION | MB_OK
									);
		return FALSE;
	}

    // 9 April 2008 jdp
    // turn off capslock if on
    ClearKeyState(VK_CAPITAL);
	vnclog.Print(LL_ALL, VNCLOG("generating ctrl-alt-del\n"));

	// Fake a hotkey event to any windows we find there.... :(
	// Winlogon uses hotkeys to trap Ctrl-Alt-Del...
	PostMessage(HWND_BROADCAST, WM_HOTKEY, 0, MAKELONG(MOD_ALT | MOD_CONTROL, VK_DELETE));

	// Switch back to our original desktop
	if (old_desktop != NULL)
			vncService::SelectHDESK(old_desktop);
	return NULL;
}

// Static routine to simulate Ctrl-Alt-Del locally

BOOL
vncService::SimulateCtrlAltDel()
{
	vnclog.Print(LL_ALL, VNCLOG("preparing to generate ctrl-alt-del\n"));

	// Are we running on NT?
	if (IsWinNT())
	{
		vnclog.Print(LL_ALL, VNCLOG("spawn ctrl-alt-del thread...\n"));

		// *** This is an unpleasant hack.  Oh dear.

		// I simulate CtrAltDel by posting a WM_HOTKEY message to all
		// the windows on the Winlogon desktop.
		// This requires that the current thread is part of the Winlogon desktop.
		// But the current thread has hooks set & a window open, so it can't
		// switch desktops, so I instead spawn a new thread & let that do the work...

		omni_thread *thread = omni_thread::create(SimulateCtrlAltDelThreadFn);
		if (thread == NULL)
			return FALSE;
		thread->join(NULL);

		return TRUE;
	}

	return TRUE;
}

// Static routine to lock a 2K or above workstation

BOOL
vncService::LockWorkstation()
{
	if (!IsWinNT()) {
		vnclog.Print(LL_INTERR, VNCLOG("unable to lock workstation - not NT\n"));
		return FALSE;
	}

	vnclog.Print(LL_ALL, VNCLOG("locking workstation\n"));

	// Load the user32 library
	HMODULE user32 = LoadLibrary("user32.dll");
	if (!user32) {
		vnclog.Print(LL_INTERR, VNCLOG("unable to load User32 DLL (%u)\n"), GetLastError());
		return FALSE;
	}

	// Get the LockWorkstation function
	typedef BOOL (*LWProc) ();
	LWProc lockworkstation = (LWProc)GetProcAddress(user32, "LockWorkStation");
	if (!lockworkstation) {
		vnclog.Print(LL_INTERR, VNCLOG("unable to locate LockWorkStation - requires Windows 2000 or above (%u)\n"), GetLastError());
		FreeLibrary(user32);
		return FALSE;
	}
	
	// Attempt to lock the workstation
	BOOL result = (lockworkstation)();

	if (!result) {
		vnclog.Print(LL_INTERR, VNCLOG("call to LockWorkstation failed\n"));
		FreeLibrary(user32);
		return FALSE;
	}

	FreeLibrary(user32);
	return result;
}

// Static routine to tell a locally-running instance of the server
// to connect out to a new client

BOOL
vncService::PostAddNewClient(unsigned long ipaddress, unsigned short port)
{
	// Post to the WinVNC menu window
	if (!PostToWinVNC(MENU_ADD_CLIENT_MSG, (WPARAM)port, (LPARAM)ipaddress))
	{

		//MessageBox(NULL, sz_ID_NO_EXIST_INST, szAppName, MB_ICONEXCLAMATION | MB_OK);

		//Little hack, seems postmessage fail in some cases on some os.
		//permission proble
		//use G_var + WM_time to reconnect
		if (port==1111 && ipaddress==1111) G_1111=true;
		return FALSE;
	}

	return TRUE;
}

// Static routine to tell a locally-running instance of the server
// about a reconnect

BOOL
vncService::PostAddAutoConnectClient( const char* pszId )
{
	ATOM aId = NULL;
	if ( pszId )
	{
		aId = GlobalAddAtom( pszId );
//		delete pszId;
	}
	return ( PostToWinVNC(MENU_AUTO_RECONNECT_MSG, NULL, (LPARAM)aId) );
}

BOOL
vncService::PostAddConnectClient( const char* pszId )
{
	ATOM aId = NULL;
	if ( pszId )
	{
		aId = GlobalAddAtom( pszId );
//		delete pszId;
	}
	return ( PostToWinVNC(MENU_REPEATER_ID_MSG, NULL, (LPARAM)aId) );
}

BOOL
vncService::RunningAsService()
{
	if (m_fRunningFromExternalService) return true;
	else return false;
}

BOOL 
vncService::RunningFromExternalService()
{
	return m_fRunningFromExternalService;
}


void 
vncService::RunningFromExternalService(BOOL fEnabled)
{
	m_fRunningFromExternalService = fEnabled;
}

////////////////////////////////////////////////////////////////////////////////
extern char service_name[];
bool
vncService::IsInstalled()
{
    BOOL bResult = FALSE;
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ENUMERATE_SERVICE); // full access
    if (hSCM) {
        SC_HANDLE hService = ::OpenService(hSCM,
                                           service_name,
                                           SERVICE_QUERY_CONFIG);
        if (hService) {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }
        ::CloseServiceHandle(hSCM);
    }
    return (FALSE != bResult);
}


