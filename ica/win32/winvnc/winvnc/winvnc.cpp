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
#include "vsocket.h"
#include "winvnc.h"

#include "vncserver.h"
#include "vncmenu.h"
#include "vncinsthandler.h"
#include "vncservice.h"
///unload driver
#include "vncOSVersion.h"
#include "videodriver.h"

#ifdef IPP
void InitIpp();
#endif

#define LOCALIZATION_MESSAGES   // ACT: full declaration instead on extern ones
#include "Localization.h" // Act : add localization on messages



// Application instance and name
HINSTANCE	hAppInstance;
const char	*szAppName = "WinVNC";
DWORD		mainthreadId;
BOOL		fRunningFromExternalService=false;

//adzm 2009-06-20
char* g_szRepeaterHost = NULL;

// sf@2007 - New shutdown order handling stuff (with uvnc_service)
bool			fShutdownOrdered = false;
static HANDLE		hShutdownEvent = NULL;
HANDLE		hShutdownEventcad = NULL;
MMRESULT			mmRes;

void WRITETOLOG(char *szText, int size, DWORD *byteswritten, void *);


//// Handle Old PostAdd message
bool PostAddAutoConnectClient_bool=false;
bool PostAddNewClient_bool=false;
bool PostAddAutoConnectClient_bool_null=false;

bool PostAddConnectClient_bool=false;
bool PostAddConnectClient_bool_null=false;

//adzm 2009-06-20
bool PostAddNewRepeaterClient_bool=false;

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
void Open_homepage();
void Open_forum();

// [v1.0.2-jp1 fix] Load resouce from dll
HINSTANCE	hInstResDLL;
BOOL SPECIAL_SC_EXIT=false;
BOOL SPECIAL_SC_PROMPT=false;
BOOL G_HTTP;
BOOL multi=false;

void Enable_softwareCAD_elevated();
void Enable_softwareCAD();
void Reboot_in_safemode_elevated();
void Reboot_in_safemode();
void delete_softwareCAD_elevated();
void delete_softwareCAD();
void Reboot_with_force_reboot_elevated();
void Reboot_with_force_reboot();

//HACK to use name in autoreconnect from service with dyn dns
char dnsname[255];

// winvnc.exe will also be used for helper exe
// This allow us to minimize the number of seperate exe
bool
Myinit(HINSTANCE hInstance)
{
	SetOSVersion();
	setbuf(stderr, 0);

	// [v1.0.2-jp1 fix] Load resouce from dll

	hInstResDLL = NULL;

	 //limit the vnclang.dll searchpath to avoid
	char szCurrentDir[MAX_PATH];
	char szCurrentDir_vnclangdll[MAX_PATH];
	if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
	{
		char* p = strrchr(szCurrentDir, '\\');
		*p = '\0';
	}
	strcpy (szCurrentDir_vnclangdll,szCurrentDir);
	strcat (szCurrentDir_vnclangdll,"\\");
	strcat (szCurrentDir_vnclangdll,"vnclang_server.dll");

	hInstResDLL = LoadLibrary(szCurrentDir_vnclangdll);

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

#ifndef ULTRAVNC_ITALC_SUPPORT
	// Save the application instance and main thread id
	hAppInstance = hInstance;
	mainthreadId = GetCurrentThreadId();


	// Initialise the VSocket system
	VSocketSystem socksys;
	if (!socksys.Initialised())
	{
		MessageBoxSecure(NULL, sz_ID_FAILED_INIT, szAppName, MB_OK);
		return 0;
	}
#endif
	return 1;
}
//#define CRASHRPT
#ifdef CRASHRPT
#include "C:/DATA/crash/crashrpt/include/crashrpt.h"
#pragma comment(lib, "C:/DATA/crash/crashrpt/lib/crashrpt")
#endif

// WinMain parses the command line and either calls the main App
// routine or, under NT, the main service routine.
int WINAPI WinMainVNC(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	// make vnc last service to stop
	SetProcessShutdownParameters(0x100,false);
	// handle dpi on aero
	HMODULE hUser32 = LoadLibrary(_T("user32.dll"));
	typedef BOOL (*SetProcessDPIAwareFunc)();
	SetProcessDPIAwareFunc setDPIAware = (SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");
	if (setDPIAware) setDPIAware();
	FreeLibrary(hUser32);

#ifdef IPP
	InitIpp();
#endif
#ifdef CRASHRPT
	Install(NULL, _T("ultravnc@skynet.be"), _T(""));
#endif
	bool Injected_autoreconnect=false;
	SPECIAL_SC_EXIT=false;
	SPECIAL_SC_PROMPT=false;
	SetOSVersion();
	setbuf(stderr, 0);

	// [v1.0.2-jp1 fix] Load resouce from dll
	hInstResDLL = NULL;

	 //limit the vnclang.dll searchpath to avoid
	char szCurrentDir[MAX_PATH];
	char szCurrentDir_vnclangdll[MAX_PATH];
	if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
	{
		char* p = strrchr(szCurrentDir, '\\');
		*p = '\0';
	}
	strcpy (szCurrentDir_vnclangdll,szCurrentDir);
	strcat (szCurrentDir_vnclangdll,"\\");
	strcat (szCurrentDir_vnclangdll,"vnclang_server.dll");

	hInstResDLL = LoadLibrary(szCurrentDir_vnclangdll);

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
		MessageBoxSecure(NULL, sz_ID_FAILED_INIT, szAppName, MB_OK);
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
			Sleep(3000);
			char mycommand[MAX_PATH];
			i+=strlen(winvncSettingshelper);
			strcpy( mycommand, &(szCmdLine[i+1]));
			Set_settings_as_admin(mycommand);
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncStopserviceHelper, strlen(winvncStopserviceHelper)) == 0)
		{
			Sleep(3000);
			Set_stop_service_as_admin();
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncKill, strlen(winvncKill)) == 0)
		{
			static HANDLE		hShutdownEventTmp;
			hShutdownEventTmp = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Global\\SessionEventUltra");
			SetEvent(hShutdownEventTmp);
			CloseHandle(hShutdownEventTmp);

			//adzm 2010-02-10 - Finds the appropriate VNC window for any process. Sends this message to all of them!
			HWND hservwnd = NULL;
			do {
				if (hservwnd!=NULL)
				{
					PostMessage(hservwnd, WM_COMMAND, 40002, 0);
					PostMessage(hservwnd, WM_CLOSE, 0, 0);
				}
				hservwnd = FindWinVNCWindow(false);
			} while (hservwnd!=NULL);
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncopenhomepage, strlen(winvncopenhomepage)) == 0)
		{
			Open_homepage();
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncopenforum, strlen(winvncopenforum)) == 0)
		{
			Open_forum();
			return 0;
		}


		if (strncmp(&szCmdLine[i], winvncStartserviceHelper, strlen(winvncStartserviceHelper)) == 0)
		{
			Sleep(3000);
			Set_start_service_as_admin();
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncInstallServiceHelper, strlen(winvncInstallServiceHelper)) == 0)
			{
				//Sleeps are realy needed, else runas fails...
				Sleep(3000);
				Set_install_service_as_admin();
				return 0;
			}
		if (strncmp(&szCmdLine[i], winvncUnInstallServiceHelper, strlen(winvncUnInstallServiceHelper)) == 0)
			{
				Sleep(3000);
				Set_uninstall_service_as_admin();
				return 0;
			}
		if (strncmp(&szCmdLine[i], winvncSoftwarecadHelper, strlen(winvncSoftwarecadHelper)) == 0)
			{
				Sleep(3000);
				Enable_softwareCAD_elevated();
				return 0;
			}
		if (strncmp(&szCmdLine[i], winvncdelSoftwarecadHelper, strlen(winvncdelSoftwarecadHelper)) == 0)
			{
				Sleep(3000);
				delete_softwareCAD_elevated();
				return 0;
			}
		if (strncmp(&szCmdLine[i], winvncRebootSafeHelper, strlen(winvncRebootSafeHelper)) == 0)
			{
				Sleep(3000);
				Reboot_in_safemode_elevated();
				return 0;
			}

		if (strncmp(&szCmdLine[i], winvncRebootForceHelper, strlen(winvncRebootForceHelper)) == 0)
			{
				Sleep(3000);
				Reboot_with_force_reboot_elevated();
				return 0;
			}

		if (strncmp(&szCmdLine[i], winvncSecurityEditorHelper, strlen(winvncSecurityEditorHelper)) == 0)
			{
				Sleep(3000);
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
						/*HRESULT hr = */CoInitialize(NULL);
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

		if (strncmp(&szCmdLine[i], winvncSoftwarecad, strlen(winvncSoftwarecad)) == 0)
		{
			Enable_softwareCAD();
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncdelSoftwarecad, strlen(winvncdelSoftwarecad)) == 0)
		{
			delete_softwareCAD();
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncRebootSafe, strlen(winvncRebootSafe)) == 0)
		{
			Reboot_in_safemode();
			return 0;
		}

		if (strncmp(&szCmdLine[i], winvncRebootForce, strlen(winvncRebootForce)) == 0)
		{
			Reboot_with_force_reboot();
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
                while (*pServiceName && !isspace(*(unsigned char*)pServiceName))
                    ++pServiceName;

                // skip past whitespace to service name
                while (*pServiceName && isspace(*(unsigned char*)pServiceName))
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
                while (*pServiceName && !isspace(*(unsigned char*)pServiceName))
                    ++pServiceName;

                // skip past whitespace to service name
                while (*pServiceName && isspace(*(unsigned char*)pServiceName))
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

		if (strncmp(&szCmdLine[i], winvncSCexit, strlen(winvncSCexit)) == 0)
		{
			SPECIAL_SC_EXIT=true;
			i+=strlen(winvncSCexit);
			continue;
		}

		if (strncmp(&szCmdLine[i], winvncSCprompt, strlen(winvncSCprompt)) == 0)
		{
			SPECIAL_SC_PROMPT=true;
			i+=strlen(winvncSCprompt);
			continue;
		}

		if (strncmp(&szCmdLine[i], winvncmulti, strlen(winvncmulti)) == 0)
		{
			multi=true;
			i+=strlen(winvncmulti);
			continue;
		}

		if (strncmp(&szCmdLine[i], winvnchttp, strlen(winvnchttp)) == 0)
		{
			G_HTTP=true;
			i+=strlen(winvnchttp);
			continue;
		}

		if (strncmp(&szCmdLine[i], winvncStopReconnect, strlen(winvncStopReconnect)) == 0)
		{
			i+=strlen(winvncStopReconnect);
			vncService::PostAddStopConnectClientAll();
			continue;
		}

		if (strncmp(&szCmdLine[i], winvncAutoReconnect, strlen(winvncAutoReconnect)) == 0)
		{
			// Note that this "autoreconnect" param MUST be BEFORE the "connect" one
			// on the command line !
			// wa@2005 -- added support for the AutoReconnectId
			i+=strlen(winvncAutoReconnect);
			Injected_autoreconnect=true;
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
				{
					strcpy(pszId_char,pszId);
					//memory leak fix
					delete [] pszId;
				}
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
				{
					strcpy(pszId_char,pszId);
					//memory leak fix
					delete [] pszId;
				}
				}
			continue;
		}

		if (strncmp(&szCmdLine[i], winvncConnect, strlen(winvncConnect)) == 0)
		{
			if (!Injected_autoreconnect)
			{
				vncService::PostAddStopConnectClient();
			}
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
					strcpy(dnsname,name);
					VCard32 address = VSocket::Resolve(name);
					delete [] name;
					if (address != 0) {
						// Post the IP address to the server
						// We can not contact a runnning service, permissions, so we must store the settings
						// and process until the vncmenu has been started
						vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient III \n"));
						if (!vncService::PostAddNewClientInit(address, port))
						{
						PostAddNewClient_bool=true;
						port_int=port;
						address_vcard=address;
						}
					}
					else
					{
						//ask for host,port
						PostAddNewClient_bool=true;
						port_int=0;
						address_vcard=0;
						Sleep(2000);
						Beep(200,1000);
						return 0;
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
				vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient IIII\n"));
				if (!vncService::PostAddNewClient(0, 0))
				{
				PostAddNewClient_bool=true;
				port_int=0;
				address_vcard=0;
				}
			}
			continue;
		}

		//adzm 2009-06-20
		if (strncmp(&szCmdLine[i], winvncRepeater, strlen(winvncRepeater)) == 0)
		{
			// set the default repeater host
			i+=strlen(winvncRepeater);

			// First, we have to parse the command line to get the host to use
			int start, end;
			start=i;
			while (szCmdLine[start] <= ' ' && szCmdLine[start] != 0) start++;
			end = start;
			while (szCmdLine[end] > ' ') end++;

			// Was there a hostname (and optionally a port number) given?
			if (end-start > 0)
			{
				if (g_szRepeaterHost) {
					delete[] g_szRepeaterHost;
					g_szRepeaterHost = NULL;
				}
				g_szRepeaterHost = new char[end-start+1];
				if (g_szRepeaterHost != 0) {
					strncpy(g_szRepeaterHost, &(szCmdLine[start]), end-start);
					g_szRepeaterHost[end-start] = 0;
					
					// We can not contact a runnning service, permissions, so we must store the settings
					// and process until the vncmenu has been started
					vnclog.Print(LL_INTERR, VNCLOG("PostAddNewRepeaterClient I\n"));
					if (!vncService::PostAddNewRepeaterClient())
					{
						PostAddNewRepeaterClient_bool=true;
						port_int=0;
						address_vcard=0;
					}
				}
				i=end;
				continue;
			}
			else 
			{
				/*
				// Tell the server to show the Add New Client dialog
				// We can not contact a runnning service, permissions, so we must store the settings
				// and process until the vncmenu has been started
				vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient IIII\n"));
				if (!vncService::PostAddNewClient(0, 0))
				{
				PostAddNewClient_bool=true;
				port_int=0;
				address_vcard=0;
				}
				*/
			}
			continue;
		}

		// Either the user gave the -help option or there is something odd on the cmd-line!

		// Show the usage dialog
		MessageBoxSecure(NULL, winvncUsageText, sz_ID_WINVNC_USAGE, MB_OK | MB_ICONINFORMATION);
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

DWORD WINAPI imp_desktop_thread(LPVOID lpParam)
{
#ifndef ULTRAVNC_ITALC_SUPPORT
	vncServer *server = (vncServer *)lpParam;
#endif
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

	char m_username[UNLEN+1];
	HWINSTA station = GetProcessWindowStation();
	if (station != NULL)
	{
		DWORD usersize;
		GetUserObjectInformation(station, UOI_USER_SID, NULL, 0, &usersize);
		SetLastError(0);
		if (usersize != 0)
		{
			DWORD length = sizeof(m_username);
			if (GetUserName(m_username, &length) == 0)
			{
				UINT error = GetLastError();
				if (error != ERROR_NOT_LOGGED_ON)
				{
					vnclog.Print(LL_INTERR, VNCLOG("getusername error %d\n"), GetLastError());
					SetThreadDesktop(old_desktop);
                	CloseDesktop(desktop);
					Sleep(500);
					return FALSE;
				}
			}
		}
	}
    vnclog.Print(LL_INTERR, VNCLOG("Username %s \n"),m_username);

#ifndef ULTRAVNC_ITALC_SUPPORT
	// Create tray icon and menu
	vncMenu *menu = new vncMenu(server);
	if (menu == NULL)
	{
		vnclog.Print(LL_INTERR, VNCLOG("failed to create tray menu\n"));
		PostQuitMessage(0);
	}
#endif

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
	{
		PostAddNewClient_bool=false;
		vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient IIIII\n"));
		vncService::PostAddNewClient(address_vcard, port_int);
	}
	//adzm 2009-06-20
	if (PostAddNewRepeaterClient_bool)
	{
		PostAddNewRepeaterClient_bool=false;
		vnclog.Print(LL_INTERR, VNCLOG("PostAddNewRepeaterClient II\n"));
		vncService::PostAddNewRepeaterClient();
	}
	bool Runonce=false;
	MSG msg;
	while (GetMessage(&msg,0,0,0) != 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (fShutdownOrdered && !Runonce)
		{
			Runonce=true;
#ifndef ULTRAVNC_ITALC_SUPPORT
			menu->Shutdown(true);
#endif
		}
	}

#ifndef ULTRAVNC_ITALC_SUPPORT
	// sf@2007 - Close all (vncMenu,tray icon, connections...)

	if (menu != NULL)
		delete menu;
#endif

	//vnclog.Print(LL_INTERR, VNCLOG("GetMessage stop \n"));
	SetThreadDesktop(old_desktop);
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
	if (mmRes != (MMRESULT)-1) return;
	vnclog.Print(LL_INTINFO, VNCLOG("****************** Init SDTimer\n"));
	mmRes = timeSetEvent( 2000, 0, (LPTIMECALLBACK)fpTimer, 0, TIME_PERIODIC );
}


void KillSDTimer()
{
	vnclog.Print(LL_INTINFO, VNCLOG("****************** Kill SDTimer\n"));
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
	if (!multi) // this allow to overwrite the multiple instance check
	{
		if (!instancehan->Init())
		{	
    		vnclog.Print(LL_ERROR, VNCLOG("%s -- exiting\n"), sz_ID_ANOTHER_INST);
			// We don't allow multiple instances!
			/*if (!fRunningFromExternalService)
				MessageBoxSecure(NULL, sz_ID_ANOTHER_INST, szAppName, MB_OK);*/
			Sleep( 5000 );
			return 0;
		}
	}

	// Initialise the VSocket system
	VSocketSystem socksys;
	if (!socksys.Initialised())
	{
		MessageBoxSecure(NULL, sz_ID_FAILED_INIT, szAppName, MB_OK);
		return 0;
	}

	//vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Previous instance checked - Trying to create server\n"));
	// CREATE SERVER
	vncServer server;

	// Set the name and port number
	server.SetName(szAppName);
	server.SockConnect( TRUE );
	vnclog.Print(LL_STATE, VNCLOG("server created ok\n"));
	///uninstall driver before cont
	
	// sf@2007 - Set Application0 special mode
	server.RunningFromExternalService(fRunningFromExternalService);

	// Create tray icon and menu
	vncMenu *menu = new vncMenu(&server);
	if (menu == NULL)
	{
		vnclog.Print(LL_INTERR, VNCLOG("failed to create tray menu\n"));
		PostQuitMessage(0);
	}


	// sf@2007 - New impersonation thread stuff for tray icon & menu
	// Subscribe to shutdown event
	hShutdownEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Global\\SessionEventUltra");
	hShutdownEventcad = OpenEvent(EVENT_MODIFY_STATE, FALSE, "Global\\SessionEventUltraCad");
	if (hShutdownEvent) ResetEvent(hShutdownEvent);
	vnclog.Print(LL_STATE, VNCLOG("***************** SDEvent created \n"));
	// Create the timer that looks periodicaly for shutdown event
	mmRes = -1;
	InitSDTimer();

	while ( !fShutdownOrdered)
	{
#ifdef ULTRAVNC_ITALC_SUPPORT
		DWORD result = WaitForSingleObject( hShutdownEvent, INFINITE );
		if( WAIT_OBJECT_0 == result )
		{
			fShutdownOrdered = true;
			server.KillAuthClients();
		}
#else
		//vnclog.Print(LL_STATE, VNCLOG("################## Creating Imp Thread : %d \n"), nn);
		HANDLE threadHandle;
		DWORD dwTId;
		threadHandle = CreateThread(NULL, 0, imp_desktop_thread, &server, 0, &dwTId);

		if (threadHandle)  
		{
			WaitForSingleObject( threadHandle, INFINITE );
			CloseHandle(threadHandle);
		}
		vnclog.Print(LL_STATE, VNCLOG("################## Closing Imp Thread\n"));
#endif
	}

	KillSDTimer();
	if (instancehan!=NULL)
		delete instancehan;

	if (hShutdownEvent)CloseHandle(hShutdownEvent);
	if (hShutdownEventcad)CloseHandle(hShutdownEventcad);
	vnclog.Print(LL_STATE, VNCLOG("################## SHUTING DOWN SERVER ####################\n"));

	//adzm 2009-06-20
	if (g_szRepeaterHost) {
		delete[] g_szRepeaterHost;
		g_szRepeaterHost = NULL;
	}
	return 1;
};
