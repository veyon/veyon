//  Copyright (C) 2007 Ultr@VNC Team Members. All Rights Reserved.
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

// WinVNC.cpp

// 24/11/97		WEZ

// WinMain and main WndProc for the new version of WinVNC

////////////////////////////
// System headers
#include "stdhdrs.h"

#include "mmsystem.h"

////////////////////////////
// Custom headers
#include "VSocket.h"
#include "WinVNC.h"

#include "vncServer.h"
#include "vncMenu.h"
#include "vncInstHandler.h"
#include "vncService.h"
///unload driver
#include "vncOSVersion.h"
#include "videodriver.h"

#ifdef IPP
void InitIpp();
#endif

#define LOCALIZATION_MESSAGES   // ACT: full declaration instead on extern ones
#include "localization.h" // Act : add localization on messages



// Application instance and name
HINSTANCE	hAppInstance;
const char	*szAppName = "WinVNC";
DWORD		mainthreadId;
BOOL		fRunningFromExternalService=false;

// sf@2007 - New shutdown order handling stuff (with uvnc_service)
bool			fShutdownOrdered = false;
static HANDLE		hShutdownEvent = NULL;
MMRESULT			mmRes;

void WRITETOLOG(char *szText, int size, DWORD *byteswritten, void *);


//// Handle Old PostAdd message
bool PostAddAutoConnectClient_bool=false;
bool PostAddNewClient_bool=false;
bool PostAddAutoConnectClient_bool_null=false;

bool PostAddConnectClient_bool=false;
bool PostAddConnectClient_bool_null=false;

char pszId_char[20];
VCard32 address_vcard;
int port_int;

int start_service(char *cmd);
int install_service(void);
int uninstall_service(void);
extern char service_name[];

void Real_stop_service();
void Set_stop_service_as_admin();
void Real_start_service();
void Set_start_service_as_admin();
void Real_settings(char *mycommand);
void Set_settings_as_admin(char *mycommand);
void Set_uninstall_service_as_admin();
void Set_install_service_as_admin();
void winvncSecurityEditorHelper_as_admin();
bool GetServiceName(TCHAR *pszAppPath, TCHAR *pszServiceName);

// [v1.0.2-jp1 fix] Load resouce from dll
HINSTANCE	hInstResDLL;

// winvnc.exe will also be used for helper exe
// This allow us to minimize the number of seperate exe
bool
Myinit(HINSTANCE hInstance)
{
	SetOSVersion();
	setbuf(stderr, 0);

	// [v1.0.2-jp1 fix] Load resouce from dll
	hInstResDLL = NULL;
	hInstResDLL = LoadLibrary("vnclang_server.dll");
	if (hInstResDLL == NULL)
	{
		hInstResDLL = hInstance;
	}
//	RegisterLinkLabel(hInstResDLL);

    //Load all messages from ressource file
    Load_Localization(hInstResDLL) ;
	vnclog.SetFile();
//	vnclog.SetMode(2);
//	vnclog.SetLevel(10);

#ifdef _DEBUG
	{
		// Get current flag
		int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

		// Turn on leak-checking bit
		tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

		// Set flag to the new value
		_CrtSetDbgFlag( tmpFlag );
	}
#endif

	// Save the application instance and main thread id
	hAppInstance = hInstance;
	mainthreadId = GetCurrentThreadId();


	// Initialise the VSocket system
	VSocketSystem socksys;
	if (!socksys.Initialised())
	{
		MessageBox(NULL, sz_ID_FAILED_INIT, szAppName, MB_OK);
		return 0;
	}
	return 1;
}
//#define CRASHRPT
#ifdef CRASHRPT
#include "C:/DATA/crash/crashrpt/include/crashrpt.h"
#pragma comment(lib, "C:/DATA/crash/crashrpt/lib/crashrpt")
#endif

// WinMain parses the command line and either calls the main App
// routine or, under NT, the main service routine.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
#ifdef IPP
	InitIpp();
#endif
#ifdef CRASHRPT
	Install(NULL, _T("ultravnc@skynet.be"), _T(""));
#endif
	SetOSVersion();
	setbuf(stderr, 0);

	// [v1.0.2-jp1 fix] Load resouce from dll
	hInstResDLL = NULL;
	hInstResDLL = LoadLibrary("vnclang_server.dll");
	if (hInstResDLL == NULL)
	{
		hInstResDLL = hInstance;
	}
//	RegisterLinkLabel(hInstResDLL);

    //Load all messages from ressource file
    Load_Localization(hInstResDLL) ;

	char WORKDIR[MAX_PATH];
	if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
		{
		char* p = strrchr(WORKDIR, '\\');
		if (p == NULL) return 0;
		*p = '\0';
		}
    char progname[MAX_PATH];
    strncpy(progname, WORKDIR, sizeof progname);
    progname[MAX_PATH - 1] = 0;
	//strcat(WORKDIR,"\\");
	//strcat(WORKDIR,"WinVNC.log");

	vnclog.SetFile();
//	vnclog.SetMode(2);
//	vnclog.SetLevel(10);

#ifdef _DEBUG
	{
		// Get current flag
		int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

		// Turn on leak-checking bit
		tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

		// Set flag to the new value
		_CrtSetDbgFlag( tmpFlag );
	}
#endif

	// Save the application instance and main thread id
	hAppInstance = hInstance;
	mainthreadId = GetCurrentThreadId();


	// Initialise the VSocket system
	VSocketSystem socksys;
	if (!socksys.Initialised())
	{
		MessageBox(NULL, sz_ID_FAILED_INIT, szAppName, MB_OK);
		return 0;
	}
    // look up the current service name in the registry.
    GetServiceName(progname, service_name);

	// Make the command-line lowercase and parse it
	size_t i;
	for (i = 0; i < strlen(szCmdLine); i++)
	{
		szCmdLine[i] = tolower(szCmdLine[i]);
	} 
	BOOL argfound = FALSE;
	for (i = 0; i < strlen(szCmdLine); i++)
	{
		if (szCmdLine[i] <= ' ')
			continue;
		argfound = TRUE;

		if (strncmp(&szCmdLine[i], winvncSettingshelper, strlen(winvncSettingshelper)) == 0)
		{
			char mycommand[MAX_PATH];
			i+=strlen(winvncSettingshelper);
			strcpy( mycommand, &(szCmdLine[i+1]));
			Set_settings_as_admin(mycommand);
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncStopserviceHelper, strlen(winvncStopserviceHelper)) == 0)
		{
			Set_stop_service_as_admin();
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncKill, strlen(winvncKill)) == 0)
		{
			static HANDLE		hShutdownEvent;
			hShutdownEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Global\\SessionEventUltra");
			SetEvent(hShutdownEvent);
			CloseHandle(hShutdownEvent);
			HWND hservwnd;
			hservwnd = FindWindow("WinVNC Tray Icon", NULL);
			if (hservwnd!=NULL)
			{
				PostMessage(hservwnd, WM_COMMAND, 40002, 0);
				PostMessage(hservwnd, WM_CLOSE, 0, 0);
			}
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncStartserviceHelper, strlen(winvncStartserviceHelper)) == 0)
		{
			Set_start_service_as_admin();
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncInstallServiceHelper, strlen(winvncInstallServiceHelper)) == 0)
			{
				Set_install_service_as_admin();
				return 0;
			}
		if (strncmp(&szCmdLine[i], winvncUnInstallServiceHelper, strlen(winvncUnInstallServiceHelper)) == 0)
			{
				Set_uninstall_service_as_admin();
				return 0;
			}
		if (strncmp(&szCmdLine[i], winvncSecurityEditorHelper, strlen(winvncSecurityEditorHelper)) == 0)
			{
				winvncSecurityEditorHelper_as_admin();
				return 0;
			}
		if (strncmp(&szCmdLine[i], winvncSecurityEditor, strlen(winvncSecurityEditor)) == 0)
			{
			    typedef void (*vncEditSecurityFn) (HWND hwnd, HINSTANCE hInstance);
				vncEditSecurityFn vncEditSecurity = 0;
				char szCurrentDir[MAX_PATH];
					if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH)) {
						char* p = strrchr(szCurrentDir, '\\');
						*p = '\0';
						strcat (szCurrentDir,"\\authSSP.dll");
					}
					HMODULE hModule = LoadLibrary(szCurrentDir);
					if (hModule) {
						vncEditSecurity = (vncEditSecurityFn) GetProcAddress(hModule, "vncEditSecurity");
						HRESULT hr = CoInitialize(NULL);
						vncEditSecurity(NULL, hAppInstance);
						CoUninitialize();
						FreeLibrary(hModule);
					}
				return 0;
			}

		if (strncmp(&szCmdLine[i], winvncSettings, strlen(winvncSettings)) == 0)
		{
			char mycommand[MAX_PATH];
			i+=strlen(winvncSettings);
			strcpy( mycommand, &(szCmdLine[i+1]));
			Real_settings(mycommand);
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncStopservice, strlen(winvncStopservice)) == 0)
		{
			Real_stop_service();
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncStartservice, strlen(winvncStartservice)) == 0)
		{
			Real_start_service();
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncInstallService, strlen(winvncInstallService)) == 0)
			{
                // rest of command line service name, if provided.
                char *pServiceName = &szCmdLine[i];
                // skip over command switch, find next whitepace
                while (*pServiceName && !isspace(*pServiceName))
                    ++pServiceName;

                // skip past whitespace to service name
                while (*pServiceName && isspace(*pServiceName))
                    ++pServiceName;

                // strip off any quotes
                if (*pServiceName && *pServiceName == '\"')
                    ++pServiceName;

                if (*pServiceName)
                {
                    // look for trailing quote, if found, terminate the string there.
                    char *pQuote = pServiceName;
                    pQuote = strrchr(pServiceName, '\"');
                    if (pQuote)
                        *pQuote = 0;
                }
                // if a service name is supplied, and it differs except in case from
                // the default, use the supplied service name instead
                if (*pServiceName && (_strcmpi(pServiceName, service_name) != 0))
                {
                    strncpy(service_name, pServiceName, 256);
                    service_name[255] = 0;
                }
				install_service();
				Sleep(2000);
				char command[MAX_PATH + 32]; // 29 January 2008 jdp
                _snprintf(command, sizeof command, "net start \"%s\"", service_name);
				WinExec(command,SW_HIDE);
				return 0;
			}
		if (strncmp(&szCmdLine[i], winvncUnInstallService, strlen(winvncUnInstallService)) == 0)
			{
				char command[MAX_PATH + 32]; // 29 January 2008 jdp 
                // rest of command line service name, if provided.
                char *pServiceName = &szCmdLine[i];
                // skip over command switch, find next whitepace
                while (*pServiceName && !isspace(*pServiceName))
                    ++pServiceName;

                // skip past whitespace to service name
                while (*pServiceName && isspace(*pServiceName))
                    ++pServiceName;

                // strip off any quotes
                if (*pServiceName && *pServiceName == '\"')
                    ++pServiceName;

                if (*pServiceName)
                {
                    // look for trailing quote, if found, terminate the string there.
                    char *pQuote = pServiceName;
                    pQuote = strrchr(pServiceName, '\"');
                    if (pQuote)
                        *pQuote = 0;
                }

                if (*pServiceName && (_strcmpi(pServiceName, service_name) != 0))
                {
                    strncpy(service_name, pServiceName, 256);
                    service_name[255] = 0;
                }
                _snprintf(command, sizeof command, "net stop \"%s\"", service_name);
				WinExec(command,SW_HIDE);
				uninstall_service();
				return 0;
			}



		if (strncmp(&szCmdLine[i], winvncRunService, strlen(winvncRunService)) == 0)
		{
			//Run as service
			if (!Myinit(hInstance)) return 0;
			fRunningFromExternalService = true;
			vncService::RunningFromExternalService(true); 
			return WinVNCAppMain();
		}

		if (strncmp(&szCmdLine[i], winvncStartService, strlen(winvncStartService)) == 0)
		{
		start_service(szCmdLine);
		return 0;
		}

		if (strncmp(&szCmdLine[i], winvncRunAsUserApp, strlen(winvncRunAsUserApp)) == 0)
		{
			// WinVNC is being run as a user-level program
			if (!Myinit(hInstance)) return 0;
			return WinVNCAppMain();
		}

		if (strncmp(&szCmdLine[i], winvncAutoReconnect, strlen(winvncAutoReconnect)) == 0)
		{
			// Note that this "autoreconnect" param MUST be BEFORE the "connect" one
			// on the command line !
			// wa@2005 -- added support for the AutoReconnectId
			i+=strlen(winvncAutoReconnect);

			int start, end;
			char* pszId = NULL;
			start = i;
			// skip any spaces and grab the parameter
			while (szCmdLine[start] <= ' ' && szCmdLine[start] != 0) start++;
			
			if ( strncmp( &szCmdLine[start], winvncAutoReconnectId, strlen(winvncAutoReconnectId) ) == 0 )
			{
				end = start;
				while (szCmdLine[end] > ' ') end++;

				pszId = new char[ end - start + 1 ];
				if (pszId != 0) 
				{
					strncpy( pszId, &(szCmdLine[start]), end - start );
					pszId[ end - start ] = 0;
					pszId = _strupr( pszId );
				}
//multiple spaces between autoreconnect and id 
				i = end;
			}// end of condition we found the ID: parameter
			
			// NOTE:  id must be NULL or the ID:???? (pointer will get deleted when message is processed)
			// We can not contact a runnning service, permissions, so we must store the settings
			// and process until the vncmenu has been started

			if (!vncService::PostAddAutoConnectClient( pszId ))
			{
				PostAddAutoConnectClient_bool=true;
				if (pszId==NULL)
				{
					PostAddAutoConnectClient_bool_null=true;
					PostAddAutoConnectClient_bool=false;
				}
				else
					strcpy(pszId_char,pszId);
			}
			continue;
		}

			
		if ( strncmp( &szCmdLine[i], winvncReconnectId, strlen(winvncReconnectId) ) == 0 )
			{
				i+=strlen("-");
				int start, end;
				char* pszId = NULL;
				start = i;
				end = start;
				while (szCmdLine[end] > ' ') end++;

				pszId = new char[ end - start + 1 ];
				if (pszId != 0) 
				{
					strncpy( pszId, &(szCmdLine[start]), end - start );
					pszId[ end - start ] = 0;
					pszId = _strupr( pszId );
				}
				i = end;
			if (!vncService::PostAddConnectClient( pszId ))
			{
				PostAddConnectClient_bool=true;
				if (pszId==NULL)
				{
					PostAddConnectClient_bool_null=true;
					PostAddConnectClient_bool=false;
				}
				else
					strcpy(pszId_char,pszId);
				}
			continue;
		}

		if (strncmp(&szCmdLine[i], winvncConnect, strlen(winvncConnect)) == 0)
		{
			// Add a new client to an existing copy of winvnc
			i+=strlen(winvncConnect);

			// First, we have to parse the command line to get the filename to use
			int start, end;
			start=i;
			while (szCmdLine[start] <= ' ' && szCmdLine[start] != 0) start++;
			end = start;
			while (szCmdLine[end] > ' ') end++;

			// Was there a hostname (and optionally a port number) given?
			if (end-start > 0)
			{
				char *name = new char[end-start+1];
				if (name != 0) {
					strncpy(name, &(szCmdLine[start]), end-start);
					name[end-start] = 0;

					int port = INCOMING_PORT_OFFSET;
					char *portp = strchr(name, ':');
					if (portp) {
						*portp++ = '\0';
						if (*portp == ':') {
							port = atoi(++portp);	// Port number after "::"
						} else {
							port = atoi(portp);	// Display number after ":"
						}
					}
					vnclog.Print(LL_STATE, VNCLOG("test... %s %d\n"),name,port);
					VCard32 address = VSocket::Resolve(name);
					delete [] name;
					if (address != 0) {
						// Post the IP address to the server
						// We can not contact a runnning service, permissions, so we must store the settings
						// and process until the vncmenu has been started
						if (!vncService::PostAddNewClient(address, port))
						{
						PostAddNewClient_bool=true;
						port_int=port;
						address_vcard=address;
						}
					}
				}
				i=end;
				continue;
			}
			else 
			{
				// Tell the server to show the Add New Client dialog
				// We can not contact a runnning service, permissions, so we must store the settings
				// and process until the vncmenu has been started
				if (!vncService::PostAddNewClient(0, 0))
				{
				PostAddNewClient_bool=true;
				port_int=0;
				address_vcard=0;
				}
			}
			continue;
		}

		// Either the user gave the -help option or there is something odd on the cmd-line!

		// Show the usage dialog
		MessageBox(NULL, winvncUsageText, sz_ID_WINVNC_USAGE, MB_OK | MB_ICONINFORMATION);
		break;
	};

	// If no arguments were given then just run
	if (!argfound)
	{
		if (!Myinit(hInstance)) return 0;
		return WinVNCAppMain();
	}

	return 0;
}


// rdv&sf@2007 - New TrayIcon impuDEsktop/impersonation thread stuff
// Todo: cleanup
HINSTANCE hInst_;
HWND hwnd_;
HANDLE Token_;
HANDLE process_;

// Todo: use same security.cpp function instead
DWORD GetCurrentUserToken_()
{
	HWND tray = FindWindow(("Shell_TrayWnd"), 0);
	if (!tray)
		return 0;
	
	DWORD processId = 0;
	GetWindowThreadProcessId(tray, &processId);
	if (!processId)
		return 0;
	
	process_ = OpenProcess(MAXIMUM_ALLOWED, FALSE, processId);
	if (!process_)
		return 0;
	
	OpenProcessToken(process_, MAXIMUM_ALLOWED, &Token_);
	return 2;
	
}

// Todo: use same security.cpp function instead
bool ImpersonateCurrentUser_()
{
  SetLastError(0);
  process_=0;
  Token_=NULL;
  if (GetCurrentUserToken_()==0)
  {
	 vnclog.Print(LL_INTERR, VNCLOG("!GetCurrentUserToken_ \n"));
     return false;
  }
  bool test=(FALSE != ImpersonateLoggedOnUser(Token_));
  if (test==1) vnclog.Print(LL_INTERR, VNCLOG("ImpersonateLoggedOnUser OK \n"));
  if (process_) CloseHandle(process_);
  if (Token_) CloseHandle(Token_);
  return test;
}


DWORD WINAPI imp_desktop_thread(LPVOID lpParam)
{
	vncServer *server = (vncServer *)lpParam;
	HDESK desktop;
	//vnclog.Print(LL_INTERR, VNCLOG("SelectDesktop \n"));
	//vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 NULL\n"));
	desktop = OpenInputDesktop(0, FALSE,
								DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
								DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
								DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
								DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
								);

	if (desktop == NULL)
		vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop Error \n"));
	else 
		vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop OK\n"));

	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
	DWORD dummy;

	char new_name[256];

	if (!GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy))
	{
		vnclog.Print(LL_INTERR, VNCLOG("!GetUserObjectInformation \n"));
	}

	vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK to %s (%x) from %x\n"), new_name, desktop, old_desktop);

	if (!SetThreadDesktop(desktop))
	{
		vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK:!SetThreadDesktop \n"));
	}

//	ImpersonateCurrentUser_();

	char m_username[200];
	HWINSTA station = GetProcessWindowStation();
	if (station != NULL)
	{
		DWORD usersize;
		GetUserObjectInformation(station, UOI_USER_SID, NULL, 0, &usersize);
		DWORD  dwErrorCode = GetLastError();
		SetLastError(0);
		if (usersize != 0)
		{
			DWORD length = usersize;
			if (GetUserName(m_username, &length) == 0)
			{
				UINT error = GetLastError();
				if (error == ERROR_NOT_LOGGED_ON)
				{
				}
				else
				{
					vnclog.Print(LL_INTERR, VNCLOG("getusername error %d\n"), GetLastError());
					return FALSE;
				}
			}
		}
	}
    vnclog.Print(LL_INTERR, VNCLOG("Username %s \n"),m_username);

	// Create tray icon and menu
	vncMenu *menu = new vncMenu(server);
	if (menu == NULL)
	{
		vnclog.Print(LL_INTERR, VNCLOG("failed to create tray menu\n"));
		PostQuitMessage(0);
	}

	// This is a good spot to handle the old PostAdd messages
	if (PostAddAutoConnectClient_bool)
		vncService::PostAddAutoConnectClient( pszId_char );
	if (PostAddAutoConnectClient_bool_null)
		vncService::PostAddAutoConnectClient( NULL );

	if (PostAddConnectClient_bool)
		vncService::PostAddConnectClient( pszId_char );
	if (PostAddConnectClient_bool_null)
		vncService::PostAddConnectClient( NULL );


	if (PostAddNewClient_bool)
	vncService::PostAddNewClient(address_vcard, port_int);

	MSG msg;
	while (GetMessage(&msg,0,0,0) != 0 && !fShutdownOrdered)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// sf@2007 - Close all (vncMenu,tray icon, connections...)
	menu->Shutdown();

	if (menu != NULL)
		delete menu;

	//vnclog.Print(LL_INTERR, VNCLOG("GetMessage stop \n"));
	CloseDesktop(desktop);
//	RevertToSelf();
	return 0;

}


// sf@2007 - For now we use a mmtimer to test the shutdown event periodically
// Maybe there's a less rude method...
void CALLBACK fpTimer(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	if (hShutdownEvent)
	{
		// vnclog.Print(LL_INTERR, VNCLOG("****************** SDTimer tic\n"));
		DWORD result=WaitForSingleObject(hShutdownEvent, 0);
		if (WAIT_OBJECT_0==result)
		{
			ResetEvent(hShutdownEvent);
			fShutdownOrdered = true;
			vnclog.Print(LL_INTERR, VNCLOG("****************** WaitForSingleObject - Shutdown server\n"));
		}
	}
}

void InitSDTimer()
{
	if (mmRes != -1) return;
	vnclog.Print(LL_INTERR, VNCLOG("****************** Init SDTimer\n"));
	mmRes = timeSetEvent( 2000, 0, (LPTIMECALLBACK)fpTimer, NULL, TIME_PERIODIC );
}


void KillSDTimer()
{
	vnclog.Print(LL_INTERR, VNCLOG("****************** Kill SDTimer\n"));
	timeKillEvent(mmRes);
	mmRes = -1;
}



// This is the main routine for WinVNC when running as an application
// (under Windows 95 or Windows NT)
// Under NT, WinVNC can also run as a service.  The WinVNCServerMain routine,
// defined in the vncService header, is used instead when running as a service.


int WinVNCAppMain()
{
	SetOSVersion();
	vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - WinVNCAPPMain\n"));
#ifdef CRASH_ENABLED
	LPVOID lpvState = Install(NULL,  "rudi.de.vos@skynet.be", "UltraVnc");
#endif

	// Set this process to be the last application to be shut down.
	// Check for previous instances of WinVNC!
	vncInstHandler *instancehan=new vncInstHandler;
	
	if (!instancehan->Init())
	{	
		// We don't allow multiple instances!
	if (!fRunningFromExternalService)
		MessageBox(NULL, sz_ID_ANOTHER_INST, szAppName, MB_OK);
		return 0;
	}

	//vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Previous instance checked - Trying to create server\n"));
	// CREATE SERVER
	vncServer server;

	// Set the name and port number
	server.SetName(szAppName);
	vnclog.Print(LL_STATE, VNCLOG("server created ok\n"));
	///uninstall driver before cont
	
	// sf@2007 - Set Application0 special mode
	server.RunningFromExternalService(fRunningFromExternalService);

	// sf@2007 - New impersonation thread stuff for tray icon & menu
	// Subscribe to shutdown event
	hShutdownEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Global\\SessionEventUltra");
	if (hShutdownEvent) ResetEvent(hShutdownEvent);
	vnclog.Print(LL_STATE, VNCLOG("***************** SDEvent created \n"));
	// Create the timer that looks periodicaly for shutdown event
	mmRes = -1;
	InitSDTimer();

	while ( !fShutdownOrdered)
	{
		//vnclog.Print(LL_STATE, VNCLOG("################## Creating Imp Thread : %d \n"), nn);

		HANDLE threadHandle;
		DWORD dwTId;
		threadHandle = CreateThread(NULL, 0, imp_desktop_thread, &server, 0, &dwTId);

		WaitForSingleObject( threadHandle, INFINITE );
		CloseHandle(threadHandle);
		vnclog.Print(LL_STATE, VNCLOG("################## Closing Imp Thread\n"));
	}

	if (instancehan!=NULL)
		delete instancehan;

	if (hShutdownEvent)CloseHandle(hShutdownEvent);
	vnclog.Print(LL_STATE, VNCLOG("################## SHUTING DOWN SERVER ####################\n"));
	return 1;
};
