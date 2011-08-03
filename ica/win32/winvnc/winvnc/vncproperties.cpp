//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 TightVNC. All Rights Reserved.
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


// vncProperties.cpp

// Implementation of the Properties dialog!

#include "stdhdrs.h"
#include "lmcons.h"
#include "vncservice.h"

#include "winvnc.h"
#include "vncproperties.h"
#include "vncserver.h"
#include "vncpasswd.h"
#include "vncOSVersion.h"
#include "common/win32_helpers.h"

#include "Localization.h" // ACT : Add localization on messages

//extern HINSTANCE g_hInst;

bool RunningAsAdministrator ();
const char WINVNC_REGISTRY_KEY [] = "Software\\ORL\\WinVNC3";

// [v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;

// Marscha@2004 - authSSP: Function pointer for dyn. linking
typedef void (*vncEditSecurityFn) (HWND hwnd, HINSTANCE hInstance);
vncEditSecurityFn vncEditSecurity = 0;
DWORD GetExplorerLogonPid();
// ethernet packet 1500 - 40 tcp/ip header - 8 PPPoE info
//unsigned int G_SENDBUFFER=8192;
unsigned int G_SENDBUFFER=1452;

// Constructor & Destructor
vncProperties::vncProperties()
{
    m_alloweditclients = TRUE;
	m_allowproperties = TRUE;
	m_allowshutdown = TRUE;
	m_dlgvisible = FALSE;
	m_usersettings = TRUE;
	Lock_service_helper=TRUE;
	m_fUseRegistry = FALSE;
    m_ftTimeout = FT_RECV_TIMEOUT;
    m_keepAliveInterval = KEEPALIVE_INTERVAL;
	m_socketKeepAliveTimeout = SOCKET_KEEPALIVE_TIMEOUT; // adzm 2010-08
	m_pref_Primary=true;
	m_pref_Secondary=false;

	m_pref_DSMPluginConfig[0] = '\0';
}

vncProperties::~vncProperties()
{
}


BOOL CALLBACK
DialogProc1(HWND hwnd,
					 UINT uMsg,
					 WPARAM wParam,
					 LPARAM lParam )
{
	switch (uMsg)
	{

	case WM_INITDIALOG:
		{

			// Show the dialog
			SetForegroundWindow(hwnd);

			return TRUE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		case IDCANCEL:
		case IDOK:
			EndDialog(hwnd, TRUE);
			return TRUE;
		}

		break;

	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		return TRUE;
	}
	return 0;
}

// Initialisation
BOOL
vncProperties::Init(vncServer *server)
{
	// Save the server pointer
	m_server = server;

	// sf@2007 - Registry mode can still be forced for backward compatibility and OS version < Vista
	m_fUseRegistry = TRUE;//((myIniFile.ReadInt("admin", "UseRegistry", 0) == 1) ? TRUE : FALSE);

	// Load the settings
	if (m_fUseRegistry)
		Load(TRUE);
	else
		LoadFromIniFile();

	// If the password is empty then always show a dialog
	char passwd[MAXPWLEN];
	m_server->GetPassword(passwd);
	if(0){
	    vncPasswd::ToText plain(passwd);
	    if (strlen(plain) == 0)
		{
			 if (!m_allowproperties || !RunningAsAdministrator ()) {
				if(m_server->AuthRequired()) {
					MessageBoxSecure(NULL, sz_ID_NO_PASSWD_NO_OVERRIDE_ERR,
								sz_ID_WINVNC_ERROR,
								MB_OK | MB_ICONSTOP);
					PostQuitMessage(0);
				} else {
					if (!vncService::RunningAsService())
						MessageBoxSecure(NULL, sz_ID_NO_PASSWD_NO_OVERRIDE_WARN,
								sz_ID_WINVNC_ERROR,
								MB_OK | MB_ICONEXCLAMATION);
				}
			} else {
				// If null passwords are not allowed, ensure that one is entered!
				if (m_server->AuthRequired()) {
					char username[UNLEN+1];
					if (!vncService::CurrentUser(username, sizeof(username)))
						return FALSE;
					if (strcmp(username, "") == 0) {
						Lock_service_helper=true;
						MessageBoxSecure(NULL, sz_ID_NO_PASSWD_NO_LOGON_WARN,
									sz_ID_WINVNC_ERROR,
									MB_OK | MB_ICONEXCLAMATION);
						ShowAdmin(TRUE, FALSE);
						Lock_service_helper=false;
					} else {
						//Warning box removed, to ugly
						//DialogBoxParam(hInstResDLL,MAKEINTRESOURCE(IDD_ABOUT1), NULL,(DLGPROC) DialogProc1,(LONG) this);
						ShowAdmin(TRUE, TRUE);
					}
				}
			}
		}
	}
	Lock_service_helper=false;
	return TRUE;
}



// Dialog box handling functions
void
vncProperties::ShowAdmin(BOOL show, BOOL usersettings)
{
//	if (Lock_service_helper) return;
	HANDLE hProcess=NULL;
	HANDLE hPToken=NULL;
#ifndef ULTRAVNC_ITALC_SUPPORT
	DWORD id=GetExplorerLogonPid();
#endif
	int iImpersonateResult=0;
	{
		char WORKDIR[MAX_PATH];
		if (!GetTempPath(MAX_PATH,WORKDIR))
			{
				//Function failed, just set something
				if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
				{
					char* p = strrchr(WORKDIR, '\\');
					if (p == NULL) return;
					*p = '\0';
				}
					strcpy(m_Tempfile,"");
					strcat(m_Tempfile,WORKDIR);//set the directory
					strcat(m_Tempfile,"\\");
					strcat(m_Tempfile,INIFILE_NAME);
		}
		else
		{
			strcpy(m_Tempfile,"");
			strcat(m_Tempfile,WORKDIR);//set the directory
			strcat(m_Tempfile,INIFILE_NAME);
		}
	}
#ifndef ULTRAVNC_ITALC_SUPPORT
	if (id!=0) 
			{
				hProcess = OpenProcess(MAXIMUM_ALLOWED,FALSE,id);
				if(OpenProcessToken(hProcess,TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY
										|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_SESSIONID
										|TOKEN_READ|TOKEN_WRITE,&hPToken))
				{
					ImpersonateLoggedOnUser(hPToken);
					iImpersonateResult = GetLastError();
					if(iImpersonateResult == ERROR_SUCCESS)
					{
						ExpandEnvironmentStringsForUser(hPToken, "%TEMP%", m_Tempfile, MAX_PATH);
						strcat(m_Tempfile,"\\");
						strcat(m_Tempfile,INIFILE_NAME);
					}
				}
	}
#endif

	if (!m_allowproperties) 
	{
		if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
		if (hProcess) CloseHandle(hProcess);
		if (hPToken) CloseHandle(hPToken);
		return;
	}
	/*if (!RunningAsAdministrator ())
		{
		if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
		CloseHandle(hProcess);
		CloseHandle(hPToken);
		return;
		}*/

	if (m_fUseRegistry)
	{
		if (vncService::RunningAsService()) usersettings=false;
		m_usersettings=usersettings;
	}

	if (show)
	{

		if (!m_fUseRegistry) // Use the ini file
		{
			// We're trying to edit the default local settings - verify that we can
			/*if (!myIniFile.IsWritable())
			{
				if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
				CloseHandle(hProcess);
				CloseHandle(hPToken);
				return;
			}*/
		}
		else // Use the registry
		{
			// Verify that we know who is logged on
			if (usersettings)
			{
				char username[UNLEN+1];
				if (!vncService::CurrentUser(username, sizeof(username)))
					{
						if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
						CloseHandle(hProcess);
						CloseHandle(hPToken);
						return;
					}
				if (strcmp(username, "") == 0) {
					MessageBoxSecure(NULL, sz_ID_NO_CURRENT_USER_ERR, sz_ID_WINVNC_ERROR, MB_OK | MB_ICONEXCLAMATION);
					if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
					CloseHandle(hProcess);
					CloseHandle(hPToken);
					return;
				}
			}
			else
			{
				// We're trying to edit the default local settings - verify that we can
				HKEY hkLocal=NULL;
				HKEY hkDefault=NULL;
				BOOL canEditDefaultPrefs = 1;
				DWORD dw;
				if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
					WINVNC_REGISTRY_KEY,
					0, REG_NONE, REG_OPTION_NON_VOLATILE,
					KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
					canEditDefaultPrefs = 0;
				else if (RegCreateKeyEx(hkLocal,
					"Default",
					0, REG_NONE, REG_OPTION_NON_VOLATILE,
					KEY_WRITE | KEY_READ, NULL, &hkDefault, &dw) != ERROR_SUCCESS)
					canEditDefaultPrefs = 0;
				if (hkLocal) RegCloseKey(hkLocal);
				if (hkDefault) RegCloseKey(hkDefault);

				if (!canEditDefaultPrefs) {
					MessageBoxSecure(NULL, sz_ID_CANNOT_EDIT_DEFAULT_PREFS, sz_ID_WINVNC_ERROR, MB_OK | MB_ICONEXCLAMATION);
					if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
					if (hProcess) CloseHandle(hProcess);
					if (hPToken) CloseHandle(hPToken);
					return;
				}
			}
		}

		// Now, if the dialog is not already displayed, show it!
		if (!m_dlgvisible)
		{
			if (m_fUseRegistry) 
			{
				if (usersettings)
					vnclog.Print(LL_INTINFO, VNCLOG("show per-user Properties\n"));
				else
					vnclog.Print(LL_INTINFO, VNCLOG("show default system Properties\n"));

				// Load in the settings relevant to the user or system
				//Load(usersettings);
				m_usersettings=usersettings;
			}

			for (;;)
			{
				m_returncode_valid = FALSE;

				// Do the dialog box
				// [v1.0.2-jp1 fix]
				//int result = DialogBoxParam(hAppInstance,
				int result = DialogBoxParam(hInstResDLL,
				    MAKEINTRESOURCE(IDD_PROPERTIES1), 
				    NULL,
				    (DLGPROC) DialogProc,
				    (LONG_PTR) this);

				if (!m_returncode_valid)
				    result = IDCANCEL;

				vnclog.Print(LL_INTINFO, VNCLOG("dialog result = %d\n"), result);

				if (result == -1)
				{
					// Dialog box failed, so quit
					PostQuitMessage(0);
					if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
					CloseHandle(hProcess);
					CloseHandle(hPToken);
					return;
				}

				// We're allowed to exit if the password is not empty
				char passwd[MAXPWLEN];
				m_server->GetPassword(passwd);
				{
				    vncPasswd::ToText plain(passwd);
				    if ((strlen(plain) != 0) || !m_server->AuthRequired())
					break;
				}

				vnclog.Print(LL_INTERR, VNCLOG("warning - empty password\n"));
#ifdef ULTRAVNC_ITALC_SUPPORT
					break;
#endif

				// The password is empty, so if OK was used then redisplay the box,
				// otherwise, if CANCEL was used, close down WinVNC
				if (result == IDCANCEL)
				{
				    vnclog.Print(LL_INTERR, VNCLOG("no password - QUITTING\n"));
				    PostQuitMessage(0);
				    if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
					CloseHandle(hProcess);
					CloseHandle(hPToken);
					return;
				}

				// If we reached here then OK was used & there is no password!
				MessageBoxSecure(NULL, sz_ID_NO_PASSWORD_WARN,
				    sz_ID_WINVNC_WARNIN, MB_OK | MB_ICONEXCLAMATION);

				omni_thread::sleep(4);
			}

			// Load in all the settings
			// If you run as service, you reload the saved settings before they are actual saved
			// via runas.....
			if (!vncService::RunningAsService())
			{
			if (m_fUseRegistry) 
				Load(TRUE);
			else
				LoadFromIniFile();
			}

		}
	}
	if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
	if (hProcess) CloseHandle(hProcess);
	if (hPToken) CloseHandle(hPToken);
}

BOOL CALLBACK
vncProperties::DialogProc(HWND hwnd,
						  UINT uMsg,
						  WPARAM wParam,
						  LPARAM lParam )
{
	// We use the dialog-box's USERDATA to store a _this pointer
	// This is set only once WM_INITDIALOG has been recieved, though!
     vncProperties *_this = helper::SafeGetWindowUserData<vncProperties>(hwnd);

	switch (uMsg)
	{

	case WM_INITDIALOG:
		{			
			vnclog.Print(LL_INTINFO, VNCLOG("INITDIALOG properties\n"));
			// Retrieve the Dialog box parameter and use it as a pointer
			// to the calling vncProperties object
            helper::SafeSetWindowUserData(hwnd, lParam);

			_this = (vncProperties *) lParam;
			_this->m_dlgvisible = TRUE;
			if (_this->m_fUseRegistry)
			{
				_this->Load(_this->m_usersettings);

				// Set the dialog box's title to indicate which Properties we're editting
				if (_this->m_usersettings) {
					SetWindowText(hwnd, sz_ID_CURRENT_USER_PROP);
				} else {
					SetWindowText(hwnd, sz_ID_DEFAULT_SYST_PROP);
				}
			}
			else
			{
				_this->LoadFromIniFile();
			}

			// Initialise the properties controls
			HWND hConnectSock = GetDlgItem(hwnd, IDC_CONNECT_SOCK);

			// Tight 1.2.7 method
			BOOL bConnectSock = _this->m_server->SockConnected();
			SendMessage(hConnectSock, BM_SETCHECK, bConnectSock, 0);

			// Set the content of the password field to a predefined string.
		    SetDlgItemText(hwnd, IDC_PASSWORD, "~~~~~~~~");
			EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), bConnectSock);

			// Set the content of the view-only password field to a predefined string. //PGM
		    SetDlgItemText(hwnd, IDC_PASSWORD2, "~~~~~~~~"); //PGM
			EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD2), bConnectSock); //PGM

			// Set the initial keyboard focus
			if (bConnectSock)
			{
				SetFocus(GetDlgItem(hwnd, IDC_PASSWORD));
				SendDlgItemMessage(hwnd, IDC_PASSWORD, EM_SETSEL, 0, (LPARAM)-1);
			}
			else
				SetFocus(hConnectSock);
			// Set display/ports settings
			_this->InitPortSettings(hwnd);

			HWND hConnectHTTP = GetDlgItem(hwnd, IDC_CONNECT_HTTP);
			SendMessage(hConnectHTTP,
				BM_SETCHECK,
				_this->m_server->HTTPConnectEnabled(),
				0);
//			HWND hConnectXDMCP = GetDlgItem(hwnd, IDC_CONNECT_XDMCP);
//			SendMessage(hConnectXDMCP,
//				BM_SETCHECK,
//				_this->m_server->XDMCPConnectEnabled(),
//				0);

			// Modif sf@2002
//		   HWND hSingleWindow = GetDlgItem(hwnd, IDC_SINGLE_WINDOW);
//           SendMessage(hSingleWindow, BM_SETCHECK, _this->m_server->SingleWindow(), 0);

		   // handler to get window name
//           HWND hWindowName = GetDlgItem(hwnd, IDC_NAME_APPLI);
//           if ( _this->m_server->GetWindowName() != NULL)
//		   {
  //             SetDlgItemText(hwnd, IDC_NAME_APPLI,_this->m_server->GetWindowName());
    //       }
    //     EnableWindow(hWindowName, _this->m_server->SingleWindow());

		   // Modif sf@2002 - v1.1.0
		   HWND hFileTransfer = GetDlgItem(hwnd, IDC_FILETRANSFER);
           SendMessage(hFileTransfer, BM_SETCHECK, _this->m_server->FileTransferEnabled(), 0);

		   HWND hFileTransferUserImp = GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK);
           SendMessage(hFileTransferUserImp, BM_SETCHECK, _this->m_server->FTUserImpersonation(), 0);
		   
		   HWND hBlank = GetDlgItem(hwnd, IDC_BLANK);
           SendMessage(hBlank, BM_SETCHECK, _this->m_server->BlankMonitorEnabled(), 0);

		   HWND hBlank2 = GetDlgItem(hwnd, IDC_BLANK2); //PGM
           SendMessage(hBlank2, BM_SETCHECK, _this->m_server->BlankInputsOnly(), 0); //PGM

		   HWND hAlpha = GetDlgItem(hwnd, IDC_ALPHA);
           SendMessage(hAlpha, BM_SETCHECK, _this->m_server->CaptureAlphaBlending(), 0);
		   HWND hAlphab = GetDlgItem(hwnd, IDC_ALPHABLACK);
           SendMessage(hAlphab, BM_SETCHECK, _this->m_server->BlackAlphaBlending(), 0);

		   // [v1.0.2-jp1 fix]
//		   HWND hGammaGray = GetDlgItem(hwnd, IDC_GAMMAGRAY);
//         SendMessage(hGammaGray, BM_SETCHECK, _this->m_server->GammaGray(), 0);
		   
		   HWND hLoopback = GetDlgItem(hwnd, IDC_ALLOWLOOPBACK);
		   BOOL fLoopback = _this->m_server->LoopbackOk();
		   SendMessage(hLoopback, BM_SETCHECK, fLoopback, 0);

		   HWND hLoopbackonly = GetDlgItem(hwnd, IDC_LOOPBACKONLY);
		   BOOL fLoopbackonly = _this->m_server->LoopbackOnly();
		   SendMessage(hLoopbackonly, BM_SETCHECK, fLoopbackonly, 0);

		   HWND hTrayicon = GetDlgItem(hwnd, IDC_DISABLETRAY);
		   BOOL fTrayicon = _this->m_server->GetDisableTrayIcon();
		   SendMessage(hTrayicon, BM_SETCHECK, fTrayicon, 0);

		   HWND hAllowshutdown = GetDlgItem(hwnd, IDC_ALLOWSHUTDOWN);
		   SendMessage(hAllowshutdown, BM_SETCHECK, !_this->m_allowshutdown , 0);

		   HWND hm_alloweditclients = GetDlgItem(hwnd, IDC_ALLOWEDITCLIENTS);
		   SendMessage(hm_alloweditclients, BM_SETCHECK, !_this->m_alloweditclients , 0);
		   _this->m_server->SetAllowEditClients(_this->m_alloweditclients);
		   

		   if (vnclog.GetMode() >= 2)
			   CheckDlgButton(hwnd, IDC_LOG, BST_CHECKED);
		   else
			   CheckDlgButton(hwnd, IDC_LOG, BST_UNCHECKED);

#ifndef AVILOG
		   ShowWindow (GetDlgItem(hwnd, IDC_CLEAR), SW_HIDE);
		   ShowWindow (GetDlgItem(hwnd, IDC_VIDEO), SW_HIDE);
#endif
		   if (vnclog.GetVideo())
		   {
			   SetDlgItemText(hwnd, IDC_EDIT_PATH, vnclog.GetPath());
			   EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), true);
			   CheckDlgButton(hwnd, IDC_VIDEO, BST_CHECKED);
		   }
		   else
		   {
			   SetDlgItemText(hwnd, IDC_EDIT_PATH, vnclog.GetPath());
			   EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), false);
			   CheckDlgButton(hwnd, IDC_VIDEO, BST_UNCHECKED);
		   }
		   
			// Marscha@2004 - authSSP: moved MS-Logon checkbox back to admin props page
			// added New MS-Logon checkbox
			// only enable New MS-Logon checkbox and Configure MS-Logon groups when MS-Logon
			// is checked.
			HWND hMSLogon = GetDlgItem(hwnd, IDC_MSLOGON_CHECKD);
			SendMessage(hMSLogon, BM_SETCHECK, _this->m_server->MSLogonRequired(), 0);

			HWND hNewMSLogon = GetDlgItem(hwnd, IDC_NEW_MSLOGON);
			SendMessage(hNewMSLogon, BM_SETCHECK, _this->m_server->GetNewMSLogon(), 0);

			EnableWindow(GetDlgItem(hwnd, IDC_NEW_MSLOGON), _this->m_server->MSLogonRequired());
			EnableWindow(GetDlgItem(hwnd, IDC_MSLOGON), _this->m_server->MSLogonRequired());
			// Marscha@2004 - authSSP: end of change

		   SetDlgItemInt(hwnd, IDC_SCALE, _this->m_server->GetDefaultScale(), false);

		   
		   // Remote input settings
			HWND hEnableRemoteInputs = GetDlgItem(hwnd, IDC_DISABLE_INPUTS);
			SendMessage(hEnableRemoteInputs,
				BM_SETCHECK,
				!(_this->m_server->RemoteInputsEnabled()),
				0);

			// Local input settings
			HWND hDisableLocalInputs = GetDlgItem(hwnd, IDC_DISABLE_LOCAL_INPUTS);
			SendMessage(hDisableLocalInputs,
				BM_SETCHECK,
				_this->m_server->LocalInputsDisabled(),
				0);

			// japanese keybaord
			HWND hJapInputs = GetDlgItem(hwnd, IDC_JAP_INPUTS);
			SendMessage(hJapInputs,
				BM_SETCHECK,
				_this->m_server->JapInputEnabled(),
				0);

			// Remove the wallpaper
			HWND hRemoveWallpaper = GetDlgItem(hwnd, IDC_REMOVE_WALLPAPER);
			SendMessage(hRemoveWallpaper,
				BM_SETCHECK,
				_this->m_server->RemoveWallpaperEnabled(),
				0);
			// Remove the composit desktop
			HWND hRemoveAero = GetDlgItem(hwnd, IDC_REMOVE_Aero);
			SendMessage(hRemoveAero,
				BM_SETCHECK,
				_this->m_server->RemoveAeroEnabled(),
				0);

			// Lock settings
			HWND hLockSetting;
			switch (_this->m_server->LockSettings()) {
			case 1:
				hLockSetting = GetDlgItem(hwnd, IDC_LOCKSETTING_LOCK);
				break;
			case 2:
				hLockSetting = GetDlgItem(hwnd, IDC_LOCKSETTING_LOGOFF);
				break;
			default:
				hLockSetting = GetDlgItem(hwnd, IDC_LOCKSETTING_NOTHING);
			};
			SendMessage(hLockSetting,
				BM_SETCHECK,
				TRUE,
				0);

			HWND hmvSetting = 0;
			switch (_this->m_server->ConnectPriority()) {
			case 0:
				hmvSetting = GetDlgItem(hwnd, IDC_MV1);
				break;
			case 1:
				hmvSetting = GetDlgItem(hwnd, IDC_MV2);
				break;
			case 2:
				hmvSetting = GetDlgItem(hwnd, IDC_MV3);
				break;
			case 3:
				hmvSetting = GetDlgItem(hwnd, IDC_MV4);
				break;
			};
			SendMessage(hmvSetting,
				BM_SETCHECK,
				TRUE,
				0);


			HWND hQuerySetting;
			switch (_this->m_server->QueryAccept()) {
			case 0:
				hQuerySetting = GetDlgItem(hwnd, IDC_DREFUSE);
				break;
			case 1:
				hQuerySetting = GetDlgItem(hwnd, IDC_DACCEPT);
				break;
			default:
				hQuerySetting = GetDlgItem(hwnd, IDC_DREFUSE);
			};
			SendMessage(hQuerySetting,
				BM_SETCHECK,
				TRUE,
				0);

			// sf@2002 - List available DSM Plugins
			HWND hPlugins = GetDlgItem(hwnd, IDC_PLUGINS_COMBO);
			int nPlugins = _this->m_server->GetDSMPluginPointer()->ListPlugins(hPlugins);
			if (!nPlugins) 
			{
				SendMessage(hPlugins, CB_ADDSTRING, 0, (LPARAM) sz_ID_NO_PLUGIN_DETECT);
				SendMessage(hPlugins, CB_SETCURSEL, 0, 0);
			}
			else
				SendMessage(hPlugins, CB_SELECTSTRING, 0, (LPARAM)_this->m_server->GetDSMPluginName());

			// Modif sf@2002
			HWND hUsePlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
			SendMessage(hUsePlugin,
				BM_SETCHECK,
				_this->m_server->IsDSMPluginEnabled(),
				0);
			HWND hButton = GetDlgItem(hwnd, IDC_PLUGIN_BUTTON);
			EnableWindow(hButton, _this->m_server->IsDSMPluginEnabled());

			// Query window option - Taken from TightVNC advanced properties 
			HWND hQuery = GetDlgItem(hwnd, IDQUERY);
			BOOL queryEnabled = (_this->m_server->QuerySetting() == 4);
			SendMessage(hQuery, BM_SETCHECK, queryEnabled, 0);
			HWND hQueryTimeout = GetDlgItem(hwnd, IDQUERYTIMEOUT);
			EnableWindow(hQueryTimeout, queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_DREFUSE), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_DACCEPT), queryEnabled);


			char timeout[128];
			UINT t = _this->m_server->QueryTimeout();
			sprintf(timeout, "%d", (int)t);
		    SetDlgItemText(hwnd, IDQUERYTIMEOUT, (const char *) timeout);

			// 2006 - Patch from KP774 - disable some options depending on this OS version
			// for Win9x, no user impersonation, no LockWorkstation
			if(OSversion() == 4 || OSversion() == 5)
			{
				// Disable userimpersonation
				_this->m_server->FTUserImpersonation(FALSE);
				EnableWindow(hFileTransferUserImp, FALSE);
				SendMessage(hFileTransferUserImp, BM_SETCHECK, FALSE, 0);

				// Disable Lock Workstation
				if(_this->m_server->LockSettings() == 1)
				{
					SendMessage(GetDlgItem(hwnd, IDC_LOCKSETTING_LOCK), BM_SETCHECK, FALSE, 0);
					_this->m_server->SetLockSettings(0);
					SendMessage(GetDlgItem(hwnd, IDC_LOCKSETTING_NOTHING), BM_SETCHECK, TRUE, 0);
				}
				EnableWindow(GetDlgItem(hwnd, IDC_LOCKSETTING_LOCK), FALSE);
			}

			// if not XP or above (if win9x or NT4 or NT3.51), disable Alpha blending
			if(!(OSversion() == 1 || OSversion()==2))
			{
				// Disable Capture Alpha Blending
				_this->m_server->CaptureAlphaBlending(FALSE);
				EnableWindow(hAlpha, FALSE);
				SendMessage(hAlpha, BM_SETCHECK, FALSE, 0);

				// Disable Alpha Blending Monitor Blanking
				_this->m_server->BlackAlphaBlending(FALSE);
				EnableWindow(hAlphab, FALSE);
				SendMessage(hAlphab, BM_SETCHECK, FALSE, 0);
			}

			SetForegroundWindow(hwnd);

			return FALSE; // Because we've set the focus
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		case IDOK:
		case IDC_APPLY:
			{
				char path[512];
				int lenpath = GetDlgItemText(hwnd, IDC_EDIT_PATH, (LPSTR) &path, 512);
				if (lenpath != 0)
					{
						vnclog.SetPath(path);
				}
				else
				{
					strcpy(path,"");
					vnclog.SetPath(path);
				}

				// Save the password
				char passwd[MAXPWLEN+1];
				// TightVNC method
				int len = GetDlgItemText(hwnd, IDC_PASSWORD, (LPSTR) &passwd, MAXPWLEN+1);
				if (strcmp(passwd, "~~~~~~~~") != 0) {
					if (len == 0)
					{
						vncPasswd::FromClear crypt;
						_this->m_server->SetPassword(crypt);
					}
					else
					{
						vncPasswd::FromText crypt(passwd);
						_this->m_server->SetPassword(crypt);
					}
				}

				memset(passwd, '\0', MAXPWLEN+1); //PGM
				len = 0; //PGM
				len = GetDlgItemText(hwnd, IDC_PASSWORD2, (LPSTR) &passwd, MAXPWLEN+1); //PGM
				if (strcmp(passwd, "~~~~~~~~") != 0) { //PGM
					if (len == 0) //PGM
					{ //PGM
						vncPasswd::FromClear crypt2; //PGM
						_this->m_server->SetPassword2(crypt2); //PGM
					} //PGM
					else //PGM
					{ //PGM
						vncPasswd::FromText crypt2(passwd); //PGM
						_this->m_server->SetPassword2(crypt2); //PGM
					} //PGM
				} //PGM

				memset(passwd, '\0', MAXPWLEN+1); //PGM
				len = 0; //PGM
				len = GetDlgItemText(hwnd, IDC_PASSWORD2, (LPSTR) &passwd, MAXPWLEN+1); //PGM
				if (strcmp(passwd, "~~~~~~~~") != 0) { //PGM
					if (len == 0) //PGM
					{ //PGM
						vncPasswd::FromClear crypt2; //PGM
						_this->m_server->SetPassword2(crypt2); //PGM
					} //PGM
					else //PGM
					{ //PGM
						vncPasswd::FromText crypt2(passwd); //PGM
						_this->m_server->SetPassword2(crypt2); //PGM
					} //PGM
				} //PGM

				// Save the new settings to the server
				int state = SendDlgItemMessage(hwnd, IDC_PORTNO_AUTO, BM_GETCHECK, 0, 0);
				_this->m_server->SetAutoPortSelect(state == BST_CHECKED);

				// Save port numbers if we're not auto selecting
				if (!_this->m_server->AutoPortSelect()) {
					if ( SendDlgItemMessage(hwnd, IDC_SPECDISPLAY,
											BM_GETCHECK, 0, 0) == BST_CHECKED ) {
						// Display number was specified
						BOOL ok;
						UINT display = GetDlgItemInt(hwnd, IDC_DISPLAYNO, &ok, TRUE);
						if (ok)
							_this->m_server->SetPorts(DISPLAY_TO_PORT(display),
													  DISPLAY_TO_HPORT(display));
					} else {
						// Assuming that port numbers were specified
						BOOL ok1, ok2;
						UINT port_rfb = GetDlgItemInt(hwnd, IDC_PORTRFB, &ok1, TRUE);
						UINT port_http = GetDlgItemInt(hwnd, IDC_PORTHTTP, &ok2, TRUE);
						if (ok1 && ok2)
							_this->m_server->SetPorts(port_rfb, port_http);
					}
				}
				HWND hConnectSock = GetDlgItem(hwnd, IDC_CONNECT_SOCK);
				_this->m_server->SockConnect(
					SendMessage(hConnectSock, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				// Update display/port controls on pressing the "Apply" button
				if (LOWORD(wParam) == IDC_APPLY)
					_this->InitPortSettings(hwnd);

				

				HWND hConnectHTTP = GetDlgItem(hwnd, IDC_CONNECT_HTTP);
				_this->m_server->EnableHTTPConnect(
					SendMessage(hConnectHTTP, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				HWND hConnectXDMCP = GetDlgItem(hwnd, IDC_CONNECT_XDMCP);
				_this->m_server->EnableXDMCPConnect(
					SendMessage(hConnectXDMCP, BM_GETCHECK, 0, 0) == BST_CHECKED
					);
				
				// Remote input stuff
				HWND hEnableRemoteInputs = GetDlgItem(hwnd, IDC_DISABLE_INPUTS);
				_this->m_server->EnableRemoteInputs(
					SendMessage(hEnableRemoteInputs, BM_GETCHECK, 0, 0) != BST_CHECKED
					);

				// Local input stuff
				HWND hDisableLocalInputs = GetDlgItem(hwnd, IDC_DISABLE_LOCAL_INPUTS);
				_this->m_server->DisableLocalInputs(
					SendMessage(hDisableLocalInputs, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				// japanese keyboard
				HWND hJapInputs = GetDlgItem(hwnd, IDC_JAP_INPUTS);
				_this->m_server->EnableJapInput(
					SendMessage(hJapInputs, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				// Wallpaper handling
				HWND hRemoveWallpaper = GetDlgItem(hwnd, IDC_REMOVE_WALLPAPER);
				_this->m_server->EnableRemoveWallpaper(
					SendMessage(hRemoveWallpaper, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				// Aero handling
				HWND hRemoveAero = GetDlgItem(hwnd, IDC_REMOVE_Aero);
				_this->m_server->EnableRemoveAero(
					SendMessage(hRemoveAero, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				// Lock settings handling
				if (SendMessage(GetDlgItem(hwnd, IDC_LOCKSETTING_LOCK), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetLockSettings(1);
				} else if (SendMessage(GetDlgItem(hwnd, IDC_LOCKSETTING_LOGOFF), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetLockSettings(2);
				} else {
					_this->m_server->SetLockSettings(0);
				}

				if (SendMessage(GetDlgItem(hwnd, IDC_DREFUSE), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetQueryAccept(0);
				} else if (SendMessage(GetDlgItem(hwnd, IDC_DACCEPT), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetQueryAccept(1);
				} 

				if (SendMessage(GetDlgItem(hwnd, IDC_MV1), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetConnectPriority(0);
				} else if (SendMessage(GetDlgItem(hwnd, IDC_MV2), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetConnectPriority(1);
				} 
				 else if (SendMessage(GetDlgItem(hwnd, IDC_MV3), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetConnectPriority(2);
				} else if (SendMessage(GetDlgItem(hwnd, IDC_MV4), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetConnectPriority(3);
				} 

				

				// Modif sf@2002
				// [v1.0.2-jp2 fix-->] Move to vncpropertiesPoll.cpp
//				HWND hSingleWindow = GetDlgItem(hwnd, IDC_SINGLE_WINDOW);
//				_this->m_server->SingleWindow(SendMessage(hSingleWindow, BM_GETCHECK, 0, 0) == BST_CHECKED);
//
//				char szName[32];
//				if (GetDlgItemText(hwnd, IDC_NAME_APPLI, (LPSTR) szName, 32) == 0)
//				{
//					vnclog.Print(LL_INTINFO,VNCLOG("Error while reading Window Name %d \n"), GetLastError());
//				}
//				else
//				{
//					_this->m_server->SetSingleWindowName(szName);
//				}
				// [<--v1.0.2-jp2 fix] Move to vncpropertiesPoll.cpp
				
				// Modif sf@2002 - v1.1.0
				HWND hFileTransfer = GetDlgItem(hwnd, IDC_FILETRANSFER);
				_this->m_server->EnableFileTransfer(SendMessage(hFileTransfer, BM_GETCHECK, 0, 0) == BST_CHECKED);

				HWND hFileTransferUserImp = GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK);
				_this->m_server->FTUserImpersonation(SendMessage(hFileTransferUserImp, BM_GETCHECK, 0, 0) == BST_CHECKED);

				HWND hBlank = GetDlgItem(hwnd, IDC_BLANK);
				_this->m_server->BlankMonitorEnabled(SendMessage(hBlank, BM_GETCHECK, 0, 0) == BST_CHECKED);
				HWND hBlank2 = GetDlgItem(hwnd, IDC_BLANK2); //PGM
				_this->m_server->BlankInputsOnly(SendMessage(hBlank2, BM_GETCHECK, 0, 0) == BST_CHECKED); //PGM
				HWND hAlpha = GetDlgItem(hwnd, IDC_ALPHA);
				_this->m_server->CaptureAlphaBlending(SendMessage(hAlpha, BM_GETCHECK, 0, 0) == BST_CHECKED);
				HWND hAlphab = GetDlgItem(hwnd, IDC_ALPHABLACK);
				_this->m_server->BlackAlphaBlending(SendMessage(hAlphab, BM_GETCHECK, 0, 0) == BST_CHECKED);

				// [v1.0.2-jp1 fix]
//				HWND hGammaGray = GetDlgItem(hwnd, IDC_GAMMAGRAY);
//				_this->m_server->GammaGray(SendMessage(hGammaGray, BM_GETCHECK, 0, 0) == BST_CHECKED);

				_this->m_server->SetLoopbackOk(IsDlgButtonChecked(hwnd, IDC_ALLOWLOOPBACK));
				_this->m_server->SetLoopbackOnly(IsDlgButtonChecked(hwnd, IDC_LOOPBACKONLY));

				_this->m_server->SetDisableTrayIcon(IsDlgButtonChecked(hwnd, IDC_DISABLETRAY));
				_this->m_allowshutdown=!IsDlgButtonChecked(hwnd, IDC_ALLOWSHUTDOWN);
				_this->m_alloweditclients=!IsDlgButtonChecked(hwnd, IDC_ALLOWEDITCLIENTS);
				_this->m_server->SetAllowEditClients(_this->m_alloweditclients);
				if (IsDlgButtonChecked(hwnd, IDC_LOG))
				{
					vnclog.SetMode(2);
					vnclog.SetLevel(10);
				}
				else
				{
					vnclog.SetMode(0);
				}
				if (IsDlgButtonChecked(hwnd, IDC_VIDEO))
				{
					vnclog.SetVideo(true);
				}
				else
				{
					vnclog.SetVideo(false);
				}
				// Modif sf@2002 - v1.1.0
				// Marscha@2004 - authSSP: moved MS-Logon checkbox back to admin props page
				// added New MS-Logon checkbox
				HWND hMSLogon = GetDlgItem(hwnd, IDC_MSLOGON_CHECKD);
				_this->m_server->RequireMSLogon(SendMessage(hMSLogon, BM_GETCHECK, 0, 0) == BST_CHECKED);
				
				HWND hNewMSLogon = GetDlgItem(hwnd, IDC_NEW_MSLOGON);
				_this->m_server->SetNewMSLogon(SendMessage(hNewMSLogon, BM_GETCHECK, 0, 0) == BST_CHECKED);
				// Marscha@2004 - authSSP: end of change

				int nDScale = GetDlgItemInt(hwnd, IDC_SCALE, NULL, FALSE);
				if (nDScale < 1 || nDScale > 9) nDScale = 1;
				_this->m_server->SetDefaultScale(nDScale);
				
				// sf@2002 - DSM Plugin loading
				// If Use plugin is checked, load the plugin if necessary
				HWND hPlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
				if (SendMessage(hPlugin, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					TCHAR szPlugin[MAX_PATH];
					GetDlgItemText(hwnd, IDC_PLUGINS_COMBO, szPlugin, MAX_PATH);
					_this->m_server->SetDSMPluginName(szPlugin);
					_this->m_server->EnableDSMPlugin(true);
				}
				else // If Use plugin unchecked but the plugin is loaded, unload it
				{
					_this->m_server->EnableDSMPlugin(false);
					if (_this->m_server->GetDSMPluginPointer()->IsLoaded())
					{
						_this->m_server->GetDSMPluginPointer()->UnloadPlugin();
						_this->m_server->GetDSMPluginPointer()->SetEnabled(false);
					}	
				}

				//adzm 2010-05-12 - dsmplugin config
				_this->m_server->SetDSMPluginConfig(_this->m_pref_DSMPluginConfig);

				// Query Window options - Taken from TightVNC advanced properties
				char timeout[256];
				if (GetDlgItemText(hwnd, IDQUERYTIMEOUT, (LPSTR) &timeout, 256) == 0)
				    _this->m_server->SetQueryTimeout(atoi(timeout));
				else
				    _this->m_server->SetQueryTimeout(atoi(timeout));
				HWND hQuery = GetDlgItem(hwnd, IDQUERY);
				_this->m_server->SetQuerySetting((SendMessage(hQuery, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 4 : 2);

				// And to the registry

				/*if (!RunningAsAdministrator () && vncService::RunningAsService())
				{
					MessageBoxSecure(NULL,"Only admins are allowed to save","Warning", MB_OK | MB_ICONINFORMATION);
				}
				else*/
				{
				// Load the settings
				if (_this->m_fUseRegistry)
					_this->Save();
				else
					_this->SaveToIniFile();
				}

				// Was ok pressed?
				if (LOWORD(wParam) == IDOK)
				{
					// Yes, so close the dialog
					vnclog.Print(LL_INTINFO, VNCLOG("enddialog (OK)\n"));

					_this->m_returncode_valid = TRUE;

					EndDialog(hwnd, IDOK);
					_this->m_dlgvisible = FALSE;
				}

				_this->m_server->SetHookings();

				return TRUE;
			}

		// Modif sf@2002
		// [v1.0.2-jp2 fix-->] Move to vncpropertiesPoll.cpp
//		 case IDC_SINGLE_WINDOW:
//			 {
//				 HWND hSingleWindow = GetDlgItem(hwnd, IDC_SINGLE_WINDOW);
//				 BOOL fSingleWindow = (SendMessage(hSingleWindow, BM_GETCHECK,0, 0) == BST_CHECKED);
//				 HWND hWindowName   = GetDlgItem(hwnd, IDC_NAME_APPLI);
//				 EnableWindow(hWindowName, fSingleWindow);
//			 }
//			 return TRUE;
		// [<--v1.0.2-jp2 fix] Move to vncpropertiesPoll.cpp

		case IDCANCEL:
			vnclog.Print(LL_INTINFO, VNCLOG("enddialog (CANCEL)\n"));

			_this->m_returncode_valid = TRUE;

			EndDialog(hwnd, IDCANCEL);
			_this->m_dlgvisible = FALSE;
			return TRUE;

	    // Added Jef Fix - 5 March 2008 paquette@atnetsend.net
        case IDC_BLANK:
            {
                // only enable alpha blanking if blanking is enabled
                HWND hBlank = ::GetDlgItem(hwnd, IDC_BLANK);
                HWND hAlphab = ::GetDlgItem(hwnd, IDC_ALPHABLACK);
                ::EnableWindow(hAlphab, ::SendMessage(hBlank, BM_GETCHECK, 0, 0) == BST_CHECKED);
                HWND hBlank2 = ::GetDlgItem(hwnd, IDC_BLANK2); //PGM
                ::EnableWindow(hBlank2, ::SendMessage(hBlank, BM_GETCHECK, 0, 0) == BST_CHECKED); //PGM
            }
            break;

        case IDC_BLANK2: //PGM
            { //PGM
                // only enable alpha blanking if Disable Only Inputs is disabled //PGM
                HWND hBlank = ::GetDlgItem(hwnd, IDC_BLANK2); //PGM
                HWND hAlphab = ::GetDlgItem(hwnd, IDC_ALPHABLACK); //PGM
                ::EnableWindow(hAlphab, ::SendMessage(hBlank, BM_GETCHECK, 0, 0) == BST_UNCHECKED); //PGM
            } //PGM
            break; //PGM

		case IDC_VIDEO:
			{
				if (IsDlgButtonChecked(hwnd, IDC_VIDEO))
				   {
					   EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), true);
					   
				   }
				   else
				   {
					   EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), false);
					   
				   }
				break;
			}

		case IDC_CLEAR:
			{
				vnclog.ClearAviConfig();
				break;
			}

		case IDC_CONNECT_SOCK:
			// TightVNC 1.2.7 method
			// The user has clicked on the socket connect tickbox
			{
				BOOL bConnectSock =
					(SendDlgItemMessage(hwnd, IDC_CONNECT_SOCK,
										BM_GETCHECK, 0, 0) == BST_CHECKED);

				EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), bConnectSock);

				HWND hPortNoAuto = GetDlgItem(hwnd, IDC_PORTNO_AUTO);
				EnableWindow(hPortNoAuto, bConnectSock);
				HWND hSpecDisplay = GetDlgItem(hwnd, IDC_SPECDISPLAY);
				EnableWindow(hSpecDisplay, bConnectSock);
				HWND hSpecPort = GetDlgItem(hwnd, IDC_SPECPORT);
				EnableWindow(hSpecPort, bConnectSock);

				EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO), bConnectSock &&
					(SendMessage(hSpecDisplay, BM_GETCHECK, 0, 0) == BST_CHECKED));
				EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), bConnectSock &&
					(SendMessage(hSpecPort, BM_GETCHECK, 0, 0) == BST_CHECKED));
				EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), bConnectSock &&
					(SendMessage(hSpecPort, BM_GETCHECK, 0, 0) == BST_CHECKED));
			}
			// RealVNC method
			/*
			// The user has clicked on the socket connect tickbox
			{
				HWND hConnectSock = GetDlgItem(hwnd, IDC_CONNECT_SOCK);
				BOOL connectsockon =
					(SendMessage(hConnectSock, BM_GETCHECK, 0, 0) == BST_CHECKED);

				HWND hAutoDisplayNo = GetDlgItem(hwnd, IDC_AUTO_DISPLAY_NO);
				EnableWindow(hAutoDisplayNo, connectsockon);
			
				HWND hPortNo = GetDlgItem(hwnd, IDC_PORTNO);
				EnableWindow(hPortNo, connectsockon
					&& (SendMessage(hAutoDisplayNo, BM_GETCHECK, 0, 0) != BST_CHECKED));
			
				HWND hPassword = GetDlgItem(hwnd, IDC_PASSWORD);
				EnableWindow(hPassword, connectsockon);
			}
			*/
			return TRUE;

		// TightVNC 1.2.7 method
		case IDC_PORTNO_AUTO:
			{
				EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), FALSE);

				SetDlgItemText(hwnd, IDC_DISPLAYNO, "");
				SetDlgItemText(hwnd, IDC_PORTRFB, "");
				SetDlgItemText(hwnd, IDC_PORTHTTP, "");
			}
			return TRUE;

		case IDC_SPECDISPLAY:
			{
				EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), FALSE);

				int display = PORT_TO_DISPLAY(_this->m_server->GetPort());
				if (display < 0 || display > 99)
					display = 0;
				SetDlgItemInt(hwnd, IDC_DISPLAYNO, display, FALSE);
				SetDlgItemInt(hwnd, IDC_PORTRFB, _this->m_server->GetPort(), FALSE);
				SetDlgItemInt(hwnd, IDC_PORTHTTP, _this->m_server->GetHttpPort(), FALSE);

				SetFocus(GetDlgItem(hwnd, IDC_DISPLAYNO));
				SendDlgItemMessage(hwnd, IDC_DISPLAYNO, EM_SETSEL, 0, (LPARAM)-1);
			}
			return TRUE;

		case IDC_SPECPORT:
			{
				EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), TRUE);

				int d1 = PORT_TO_DISPLAY(_this->m_server->GetPort());
				int d2 = HPORT_TO_DISPLAY(_this->m_server->GetHttpPort());
				if (d1 == d2 && d1 >= 0 && d1 <= 99) {
					SetDlgItemInt(hwnd, IDC_DISPLAYNO, d1, FALSE);
				} else {
					SetDlgItemText(hwnd, IDC_DISPLAYNO, "");
				}
				SetDlgItemInt(hwnd, IDC_PORTRFB, _this->m_server->GetPort(), FALSE);
				SetDlgItemInt(hwnd, IDC_PORTHTTP, _this->m_server->GetHttpPort(), FALSE);

				SetFocus(GetDlgItem(hwnd, IDC_PORTRFB));
				SendDlgItemMessage(hwnd, IDC_PORTRFB, EM_SETSEL, 0, (LPARAM)-1);
			}
			return TRUE;

		// RealVNC method
		/*
		case IDC_AUTO_DISPLAY_NO:
			// User has toggled the Auto Port Select feature.
			// If this is in use, then we don't allow the Display number field
			// to be modified!
			{
				// Get the auto select button
				HWND hPortNoAuto = GetDlgItem(hwnd, IDC_AUTO_DISPLAY_NO);

				// Should the portno field be modifiable?
				BOOL enable = SendMessage(hPortNoAuto, BM_GETCHECK, 0, 0) != BST_CHECKED;

				// Set the state
				HWND hPortNo = GetDlgItem(hwnd, IDC_PORTNO);
				EnableWindow(hPortNo, enable);
			}
			return TRUE;
		*/

		// Query window option - Taken from TightVNC advanced properties code
		case IDQUERY:
			{
				HWND hQuery = GetDlgItem(hwnd, IDQUERY);
				BOOL queryon = (SendMessage(hQuery, BM_GETCHECK, 0, 0) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwnd, IDQUERYTIMEOUT), queryon);
				EnableWindow(GetDlgItem(hwnd, IDC_DREFUSE), queryon);
				EnableWindow(GetDlgItem(hwnd, IDC_DACCEPT), queryon);
			}
			return TRUE;

		// sf@2002 - DSM Plugin
		case IDC_PLUGIN_CHECK:
			{
				HWND hUse = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
				BOOL enable = SendMessage(hUse, BM_GETCHECK, 0, 0) == BST_CHECKED;
				HWND hButton = GetDlgItem(hwnd, IDC_PLUGIN_BUTTON);
				EnableWindow(hButton, enable);
			}
			return TRUE;
			// Marscha@2004 - authSSP: moved MSLogon checkbox back to admin props page
			// Reason: Different UI for old and new mslogon group config.
		case IDC_MSLOGON_CHECKD:
			{
				BOOL bMSLogonChecked =
				(SendDlgItemMessage(hwnd, IDC_MSLOGON_CHECKD,
										BM_GETCHECK, 0, 0) == BST_CHECKED);

				EnableWindow(GetDlgItem(hwnd, IDC_NEW_MSLOGON), bMSLogonChecked);
				EnableWindow(GetDlgItem(hwnd, IDC_MSLOGON), bMSLogonChecked);

			}
			return TRUE;
		case IDC_MSLOGON:
			{
				// Marscha@2004 - authSSP: if "New MS-Logon" is checked,
				// call vncEditSecurity from SecurityEditor.dll,
				// else call "old" dialog.
				BOOL bNewMSLogonChecked =
				(SendDlgItemMessage(hwnd, IDC_NEW_MSLOGON,
										BM_GETCHECK, 0, 0) == BST_CHECKED);
				if (bNewMSLogonChecked) {
					void winvncSecurityEditorHelper_as_admin();
						HANDLE hProcess,hPToken;
						DWORD id=GetExplorerLogonPid();
						if (id!=0) 
						{
							hProcess = OpenProcess(MAXIMUM_ALLOWED,FALSE,id);
							if(!OpenProcessToken(hProcess,TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY
													|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_SESSIONID
													|TOKEN_READ|TOKEN_WRITE,&hPToken)) break;

							char dir[MAX_PATH];
							char exe_file_name[MAX_PATH];
							GetModuleFileName(0, exe_file_name, MAX_PATH);
							strcpy(dir, exe_file_name);
							strcat(dir, " -securityeditorhelper");
				
							{
								STARTUPINFO          StartUPInfo;
								PROCESS_INFORMATION  ProcessInfo;
								ZeroMemory(&StartUPInfo,sizeof(STARTUPINFO));
								ZeroMemory(&ProcessInfo,sizeof(PROCESS_INFORMATION));
								StartUPInfo.wShowWindow = SW_SHOW;
								StartUPInfo.lpDesktop = "Winsta0\\Default";
								StartUPInfo.cb = sizeof(STARTUPINFO);
						
								CreateProcessAsUser(hPToken,NULL,dir,NULL,NULL,FALSE,DETACHED_PROCESS,NULL,NULL,&StartUPInfo,&ProcessInfo);
								DWORD error=GetLastError();
                                if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
                                if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
								if (error==1314)
									{
										winvncSecurityEditorHelper_as_admin();
									}

							}
						}

/*
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
						vncEditSecurity(hwnd, hAppInstance);
						CoUninitialize();
						FreeLibrary(hModule);
					}*/
				} else { 
					// Marscha@2004 - authSSP: end of change
					_this->m_vncauth.Init(_this->m_server);
					_this->m_vncauth.SetTemp(_this->m_Tempfile);
					_this->m_vncauth.Show(TRUE);
				}
			}
			return TRUE;
		case IDC_CHECKDRIVER:
			{
				CheckVideoDriver(1);
			}
			return TRUE;
		case IDC_PLUGIN_BUTTON:
			{
				HWND hPlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
				if (SendMessage(hPlugin, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					TCHAR szPlugin[MAX_PATH];
					GetDlgItemText(hwnd, IDC_PLUGINS_COMBO, szPlugin, MAX_PATH);
					if (!_this->m_server->GetDSMPluginPointer()->IsLoaded())
						_this->m_server->GetDSMPluginPointer()->LoadPlugin(szPlugin, false);
					else
					{
						// sf@2003 - We check if the loaded plugin is the same than
						// the currently selected one or not
						_this->m_server->GetDSMPluginPointer()->DescribePlugin();
						if (_stricmp(_this->m_server->GetDSMPluginPointer()->GetPluginFileName(), szPlugin))
						{
							_this->m_server->GetDSMPluginPointer()->UnloadPlugin();
							_this->m_server->GetDSMPluginPointer()->LoadPlugin(szPlugin, false);
						}
					}
				
					if (_this->m_server->GetDSMPluginPointer()->IsLoaded())
					{
						// We don't send the password yet... no matter the plugin requires
						// it or not, we will provide it later (at plugin "real" init)
						// Knowing the environnement ("server-svc" or "server-app") right 
						// now can be usefull or even mandatory for the plugin 
						// (specific params saving and so on...)
						char szParams[32];
						strcpy(szParams, "NoPassword,");
						strcat(szParams, vncService::RunningAsService() ? "server-svc" : "server-app");
						//adzm 2010-05-12 - dsmplugin config
						char* szNewConfig = NULL;
						if (_this->m_server->GetDSMPluginPointer()->SetPluginParams(hwnd, szParams, _this->m_pref_DSMPluginConfig, &szNewConfig)) {
							if (szNewConfig != NULL && strlen(szNewConfig) > 0) {
								strcpy_s(_this->m_pref_DSMPluginConfig, 511, szNewConfig);
							}
						}
					}
					else
					{
						MessageBoxSecure(NULL, 
							sz_ID_PLUGIN_NOT_LOAD, 
							sz_ID_PLUGIN_LOADIN, MB_OK | MB_ICONEXCLAMATION );
					}
				}				
				return TRUE;
			}



		}
		break;
	}
	return 0;
}



// TightVNC 1.2.7
// Set display/port settings to the correct state
void
vncProperties::InitPortSettings(HWND hwnd)
{
	BOOL bConnectSock = m_server->SockConnected();
	BOOL bAutoPort = m_server->AutoPortSelect();
	UINT port_rfb = m_server->GetPort();
	UINT port_http = m_server->GetHttpPort();
	int d1 = PORT_TO_DISPLAY(port_rfb);
	int d2 = HPORT_TO_DISPLAY(port_http);
	BOOL bValidDisplay = (d1 == d2 && d1 >= 0 && d1 <= 99);

	CheckDlgButton(hwnd, IDC_PORTNO_AUTO,
		(bAutoPort) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_SPECDISPLAY,
		(!bAutoPort && bValidDisplay) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_SPECPORT,
		(!bAutoPort && !bValidDisplay) ? BST_CHECKED : BST_UNCHECKED);

	EnableWindow(GetDlgItem(hwnd, IDC_PORTNO_AUTO), bConnectSock);
	EnableWindow(GetDlgItem(hwnd, IDC_SPECDISPLAY), bConnectSock);
	EnableWindow(GetDlgItem(hwnd, IDC_SPECPORT), bConnectSock);

	if (bValidDisplay) {
		SetDlgItemInt(hwnd, IDC_DISPLAYNO, d1, FALSE);
	} else {
		SetDlgItemText(hwnd, IDC_DISPLAYNO, "");
	}
	SetDlgItemInt(hwnd, IDC_PORTRFB, port_rfb, FALSE);
	SetDlgItemInt(hwnd, IDC_PORTHTTP, port_http, FALSE);

	EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO),
		bConnectSock && !bAutoPort && bValidDisplay);
	EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB),
		bConnectSock && !bAutoPort && !bValidDisplay);
	EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP),
		bConnectSock && !bAutoPort && !bValidDisplay);
}

#ifdef ULTRAVNC_ITALC_SUPPORT
extern BOOL ultravnc_italc_load_int( LPCSTR valname, LONG *out );
#endif

// Functions to load & save the settings
LONG
vncProperties::LoadInt(HKEY key, LPCSTR valname, LONG defval)
{
	LONG pref;
	ULONG type = REG_DWORD;
	ULONG prefsize = sizeof(pref);

#ifdef ULTRAVNC_ITALC_SUPPORT
	LONG out;
	if( ultravnc_italc_load_int( valname, &out ) )
	{
		return out;
	}
#endif
	if (RegQueryValueEx(key,
		valname,
		NULL,
		&type,
		(LPBYTE) &pref,
		&prefsize) != ERROR_SUCCESS)
		return defval;

	if (type != REG_DWORD)
		return defval;

	if (prefsize != sizeof(pref))
		return defval;

	return pref;
}

void
vncProperties::LoadPassword(HKEY key, char *buffer)
{
	DWORD type = REG_BINARY;
	int slen=MAXPWLEN;
	char inouttext[MAXPWLEN];

	// Retrieve the encrypted password
	if (RegQueryValueEx(key,
		"Password",
		NULL,
		&type,
		(LPBYTE) &inouttext,
		(LPDWORD) &slen) != ERROR_SUCCESS)
		return;

	if (slen > MAXPWLEN)
		return;

	memcpy(buffer, inouttext, MAXPWLEN);
}

void //PGM
vncProperties::LoadPassword2(HKEY key, char *buffer) //PGM
{ //PGM
	DWORD type = REG_BINARY; //PGM
	int slen=MAXPWLEN; //PGM
	char inouttext[MAXPWLEN]; //PGM

	// Retrieve the encrypted password //PGM
	if (RegQueryValueEx(key, //PGM
		"Password2", //PGM
		NULL, //PGM
		&type, //PGM
		(LPBYTE) &inouttext, //PGM
		(LPDWORD) &slen) != ERROR_SUCCESS) //PGM
		return; //PGM

	if (slen > MAXPWLEN) //PGM
		return; //PGM

	memcpy(buffer, inouttext, MAXPWLEN); //PGM
} //PGM

char *
vncProperties::LoadString(HKEY key, LPCSTR keyname)
{
	DWORD type = REG_SZ;
	DWORD buflen = 0;
	BYTE *buffer = 0;

	// Get the length of the AuthHosts string
	if (RegQueryValueEx(key,
		keyname,
		NULL,
		&type,
		NULL,
		&buflen) != ERROR_SUCCESS)
		return 0;

	if (type != REG_SZ)
		return 0;
	buffer = new BYTE[buflen];
	if (buffer == 0)
		return 0;

	// Get the AuthHosts string data
	if (RegQueryValueEx(key,
		keyname,
		NULL,
		&type,
		buffer,
		&buflen) != ERROR_SUCCESS) {
		delete [] buffer;
		return 0;
	}

	// Verify the type
	if (type != REG_SZ) {
		delete [] buffer;
		return 0;
	}

	return (char *)buffer;
}


void
vncProperties::ResetRegistry()
{	
	char username[UNLEN+1];
	HKEY hkLocal, hkLocalUser, hkDefault;
	DWORD dw;

	if (!vncService::CurrentUser((char *)&username, sizeof(username)))
		return;

	// If there is no user logged on them default to SYSTEM
	if (strcmp(username, "") == 0)
		strcpy((char *)&username, "SYSTEM");

	// Try to get the machine registry key for WinVNC
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		WINVNC_REGISTRY_KEY,
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
		{
		hkLocalUser=NULL;
		hkDefault=NULL;
		goto LABELUSERSETTINGS;
		}

	// Now try to get the per-user local key
	if (RegOpenKeyEx(hkLocal,
		username,
		0, KEY_READ,
		&hkLocalUser) != ERROR_SUCCESS)
		hkLocalUser = NULL;

	// Get the default key
	if (RegCreateKeyEx(hkLocal,
		"Default",
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ,
		NULL,
		&hkDefault,
		&dw) != ERROR_SUCCESS)
		hkDefault = NULL;

	if (hkLocalUser != NULL) RegCloseKey(hkLocalUser);
	if (hkDefault != NULL) RegCloseKey(hkDefault);
	if (hkLocal != NULL) RegCloseKey(hkLocal);
	RegCloseKey(HKEY_LOCAL_MACHINE);
LABELUSERSETTINGS:
	if ((strcmp(username, "SYSTEM") != 0))
		{
			HKEY hkGlobalUser;
			if (RegCreateKeyEx(HKEY_CURRENT_USER,
				WINVNC_REGISTRY_KEY,
				0, REG_NONE, REG_OPTION_NON_VOLATILE,
				KEY_READ, NULL, &hkGlobalUser, &dw) == ERROR_SUCCESS)
			{
				RegCloseKey(hkGlobalUser);
				RegCloseKey(HKEY_CURRENT_USER);
			}
		}

}

void
vncProperties::Load(BOOL usersettings)
{
	vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Entering Load\n"));

	//if (m_dlgvisible) {
	//	vnclog.Print(LL_INTWARN, VNCLOG("service helper invoked while Properties panel displayed\n"));
	//	return;
	//}
	ResetRegistry();

	if (vncService::RunningAsService()) usersettings=false;

	// sf@2007 - Vista mode
	// The WinVNC service mode is not used under Vista (due to Session0 isolation)
	// Default settings (Service mode) are used when WinVNC app in run under Vista login screen
	// User settings (loggued user mode) are used when WinVNC app in run in a user session
	// Todo: Maybe we should additionally check OS version...
	if (m_server->RunningFromExternalService())
		usersettings=false;

	m_usersettings = usersettings;

	if (m_usersettings)
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - User mode\n"));
	else
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Service mode\n"));
	
	char username[UNLEN+1];
	HKEY hkLocal, hkLocalUser, hkDefault;
	DWORD dw;
	
	// NEW (R3) PREFERENCES ALGORITHM
	// 1.	Look in HKEY_LOCAL_MACHINE/Software/ORL/WinVNC3/%username%
	//		for sysadmin-defined, user-specific settings.
	// 2.	If not found, fall back to %username%=Default
	// 3.	If AllowOverrides is set then load settings from
	//		HKEY_CURRENT_USER/Software/ORL/WinVNC3

	// GET THE CORRECT KEY TO READ FROM

	// Get the user name / service name
	if (!vncService::CurrentUser((char *)&username, sizeof(username)))
	{
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - NO current user\n"));
		return;
	}

	// If there is no user logged on them default to SYSTEM
	if (strcmp(username, "") == 0)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Force USER SYSTEM 1\n"));
		strcpy((char *)&username, "SYSTEM");
	}


	vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - UserName = %s\n"), username);

	// Try to get the machine registry key for WinVNC
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		WINVNC_REGISTRY_KEY,
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
		{
		hkLocalUser=NULL;
		hkDefault=NULL;
		goto LABELUSERSETTINGS;
		}

	// Now try to get the per-user local key
	if (RegOpenKeyEx(hkLocal,
		username,
		0, KEY_READ,
		&hkLocalUser) != ERROR_SUCCESS)
		hkLocalUser = NULL;

	// Get the default key
	if (RegCreateKeyEx(hkLocal,
		"Default",
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ,
		NULL,
		&hkDefault,
		&dw) != ERROR_SUCCESS)
		hkDefault = NULL;

	// LOAD THE MACHINE-LEVEL PREFS

	vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Machine level prefs\n"));

	// Logging/debugging prefs
	vnclog.Print(LL_INTINFO, VNCLOG("loading local-only settings\n"));
	//vnclog.SetMode(LoadInt(hkLocal, "DebugMode", 0));
	//vnclog.SetLevel(LoadInt(hkLocal, "DebugLevel", 0));

	// Disable Tray Icon
	m_server->SetDisableTrayIcon(LoadInt(hkLocal, "DisableTrayIcon", false));

	// Authentication required, loopback allowed, loopbackOnly

	m_server->SetLoopbackOnly(LoadInt(hkLocal, "LoopbackOnly", false));

	m_pref_RequireMSLogon=false;
	m_pref_RequireMSLogon = LoadInt(hkLocal, "MSLogonRequired", m_pref_RequireMSLogon);
	m_server->RequireMSLogon(m_pref_RequireMSLogon);

	// Marscha@2004 - authSSP: added NewMSLogon checkbox to admin props page
	m_pref_NewMSLogon = false;
	m_pref_NewMSLogon = LoadInt(hkLocal, "NewMSLogon", m_pref_NewMSLogon);
	m_server->SetNewMSLogon(m_pref_NewMSLogon);

	// sf@2003 - Moved DSM params here
	m_pref_UseDSMPlugin=false;
	m_pref_UseDSMPlugin = LoadInt(hkLocal, "UseDSMPlugin", m_pref_UseDSMPlugin);
	LoadDSMPluginName(hkLocal, m_pref_szDSMPlugin);	
	
	//adzm 2010-05-12 - dsmplugin config
	{
		char* szBuffer = LoadString(hkLocal, "DSMPluginConfig");
		if (szBuffer) {
			strncpy_s(m_pref_DSMPluginConfig, sizeof(m_pref_DSMPluginConfig) - 1, szBuffer, _TRUNCATE);
			delete[] szBuffer;
		} else {
			m_pref_DSMPluginConfig[0] = '\0';
		}
	}

	if (m_server->LoopbackOnly()) m_server->SetLoopbackOk(true);
	else m_server->SetLoopbackOk(LoadInt(hkLocal, "AllowLoopback", false));
	m_server->SetAuthRequired(LoadInt(hkLocal, "AuthRequired", true));

	m_server->SetConnectPriority(LoadInt(hkLocal, "ConnectPriority", 0));
	if (!m_server->LoopbackOnly())
	{
		char *authhosts = LoadString(hkLocal, "AuthHosts");
		if (authhosts != 0) {
			m_server->SetAuthHosts(authhosts);
			delete [] authhosts;
		} else {
			m_server->SetAuthHosts(0);
		}
	} else {
		m_server->SetAuthHosts(0);
	}

	// If Socket connections are allowed, should the HTTP server be enabled?
LABELUSERSETTINGS:
	// LOAD THE USER PREFERENCES
	vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Load User Preferences\n"));

	// Set the default user prefs
	vnclog.Print(LL_INTINFO, VNCLOG("clearing user settings\n"));
	m_pref_AutoPortSelect=TRUE;
    m_pref_HTTPConnect = TRUE;
	m_pref_XDMCPConnect = TRUE;
	m_pref_PortNumber = RFB_PORT_OFFSET; 
	m_pref_SockConnect=TRUE;
	{
	    vncPasswd::FromClear crypt;
	    memcpy(m_pref_passwd, crypt, MAXPWLEN);
	}
	m_pref_QuerySetting=2;
	m_pref_QueryTimeout=10;
	m_pref_QueryAccept=0;
	m_pref_IdleTimeout=0;
	m_pref_EnableRemoteInputs=TRUE;
	m_pref_DisableLocalInputs=FALSE;
	m_pref_EnableJapInput=FALSE;
	m_pref_clearconsole=FALSE;
	m_pref_LockSettings=-1;

	m_pref_RemoveWallpaper=TRUE;
	// adzm - 2010-07 - Disable more effects or font smoothing
	m_pref_RemoveEffects=FALSE;
	m_pref_RemoveFontSmoothing=FALSE;
	m_pref_RemoveAero=TRUE;
    m_alloweditclients = TRUE;
	m_allowshutdown = TRUE;
	m_allowproperties = TRUE;

	// Modif sf@2002
	// [v1.0.2-jp2 fix] Move to vncpropertiesPoll.cpp
//	m_pref_SingleWindow = FALSE;
	m_pref_UseDSMPlugin = FALSE;
	*m_pref_szDSMPlugin = '\0';
	m_pref_DSMPluginConfig[0] = '\0';

	m_pref_EnableFileTransfer = TRUE;
	m_pref_FTUserImpersonation = TRUE;
	m_pref_EnableBlankMonitor = TRUE;
	m_pref_BlankInputsOnly = FALSE;
	m_pref_QueryIfNoLogon = FALSE;
	m_pref_DefaultScale = 1;
	m_pref_CaptureAlphaBlending = FALSE; 
	m_pref_BlackAlphaBlending = FALSE; 
//	m_pref_GammaGray = FALSE;			// [v1.0.2-jp1 fix]


	// Load the local prefs for this user
	if (hkDefault != NULL)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Local Preferences - Default\n"));

		vnclog.Print(LL_INTINFO, VNCLOG("loading DEFAULT local settings\n"));
		LoadUserPrefs(hkDefault);
		m_allowshutdown = LoadInt(hkDefault, "AllowShutdown", m_allowshutdown);
		m_allowproperties = LoadInt(hkDefault, "AllowProperties", m_allowproperties);
		m_alloweditclients = LoadInt(hkDefault, "AllowEditClients", m_alloweditclients);
	}

	// Are we being asked to load the user settings, or just the default local system settings?
	if (usersettings)
	{
		// We want the user settings, so load them!
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - User Settings on\n"));

		if (hkLocalUser != NULL)
		{
			vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - LoadUser Preferences\n"));

			vnclog.Print(LL_INTINFO, VNCLOG("loading \"%s\" local settings\n"), username);
			LoadUserPrefs(hkLocalUser);
			m_allowshutdown = LoadInt(hkLocalUser, "AllowShutdown", m_allowshutdown);
			m_allowproperties = LoadInt(hkLocalUser, "AllowProperties", m_allowproperties);
		  m_alloweditclients = LoadInt(hkLocalUser, "AllowEditClients", m_alloweditclients);
		}

		// Now override the system settings with the user's settings
		// If the username is SYSTEM then don't try to load them, because there aren't any...
		if (m_allowproperties && (strcmp(username, "SYSTEM") != 0))
		{
			vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Override system settings with users settings\n"));
			HKEY hkGlobalUser;
			if (RegCreateKeyEx(HKEY_CURRENT_USER,
				WINVNC_REGISTRY_KEY,
				0, REG_NONE, REG_OPTION_NON_VOLATILE,
				KEY_READ, NULL, &hkGlobalUser, &dw) == ERROR_SUCCESS)
			{
				vnclog.Print(LL_INTINFO, VNCLOG("loading \"%s\" global settings\n"), username);
				LoadUserPrefs(hkGlobalUser);
				RegCloseKey(hkGlobalUser);

				// Close the user registry hive so it can unload if required
				RegCloseKey(HKEY_CURRENT_USER);
			}
		}
	} else {
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - User Settings off\n"));
		if (hkLocalUser != NULL)
		{
			vnclog.Print(LL_INTINFO, VNCLOG("loading \"%s\" local settings\n"), username);
			LoadUserPrefs(hkLocalUser);
			m_allowshutdown = LoadInt(hkLocalUser, "AllowShutdown", m_allowshutdown);
			m_allowproperties = LoadInt(hkLocalUser, "AllowProperties", m_allowproperties);
		    m_alloweditclients = LoadInt(hkLocalUser, "AllowEditClients", m_alloweditclients);
		}
		vnclog.Print(LL_INTINFO, VNCLOG("bypassing user-specific settings (both local and global)\n"));
	}

	if (hkLocalUser != NULL) RegCloseKey(hkLocalUser);
	if (hkDefault != NULL) RegCloseKey(hkDefault);
	if (hkLocal != NULL) RegCloseKey(hkLocal);

	// Make the loaded settings active..
	ApplyUserPrefs();
}

void
vncProperties::LoadUserPrefs(HKEY appkey)
{
	// LOAD USER PREFS FROM THE SELECTED KEY

	// Modif sf@2002
	m_pref_EnableFileTransfer = LoadInt(appkey, "FileTransferEnabled", m_pref_EnableFileTransfer);
	m_pref_FTUserImpersonation = LoadInt(appkey, "FTUserImpersonation", m_pref_FTUserImpersonation); // sf@2005
	m_pref_EnableBlankMonitor = LoadInt(appkey, "BlankMonitorEnabled", m_pref_EnableBlankMonitor);
	m_pref_BlankInputsOnly = LoadInt(appkey, "BlankInputsOnly", m_pref_BlankInputsOnly); //PGM
	m_pref_DefaultScale = LoadInt(appkey, "DefaultScale", m_pref_DefaultScale);
	m_pref_CaptureAlphaBlending = LoadInt(appkey, "CaptureAlphaBlending", m_pref_CaptureAlphaBlending); // sf@2005
	m_pref_BlackAlphaBlending = LoadInt(appkey, "BlackAlphaBlending", m_pref_BlackAlphaBlending); // sf@2005
	
	m_pref_Primary=LoadInt(appkey, "primary", m_pref_Primary);
	m_pref_Secondary=LoadInt(appkey, "secondary", m_pref_Secondary);

	m_pref_UseDSMPlugin = LoadInt(appkey, "UseDSMPlugin", m_pref_UseDSMPlugin);
	LoadDSMPluginName(appkey, m_pref_szDSMPlugin);

	// Connection prefs
	m_pref_SockConnect=LoadInt(appkey, "SocketConnect", m_pref_SockConnect);
	m_pref_HTTPConnect=LoadInt(appkey, "HTTPConnect", m_pref_HTTPConnect);
	m_pref_XDMCPConnect=LoadInt(appkey, "XDMCPConnect", m_pref_XDMCPConnect);
	m_pref_AutoPortSelect=LoadInt(appkey, "AutoPortSelect", m_pref_AutoPortSelect);
	m_pref_PortNumber=LoadInt(appkey, "PortNumber", m_pref_PortNumber);
	m_pref_HttpPortNumber=LoadInt(appkey, "HTTPPortNumber",
									DISPLAY_TO_HPORT(PORT_TO_DISPLAY(m_pref_PortNumber)));
	m_pref_IdleTimeout=LoadInt(appkey, "IdleTimeout", m_pref_IdleTimeout);
	
	m_pref_RemoveWallpaper=LoadInt(appkey, "RemoveWallpaper", m_pref_RemoveWallpaper);
	// adzm - 2010-07 - Disable more effects or font smoothing
	m_pref_RemoveEffects=LoadInt(appkey, "RemoveEffects", m_pref_RemoveEffects);
	m_pref_RemoveFontSmoothing=LoadInt(appkey, "RemoveFontSmoothing", m_pref_RemoveFontSmoothing);
	m_pref_RemoveAero=LoadInt(appkey, "RemoveAero", m_pref_RemoveAero);

	// Connection querying settings
	m_pref_QuerySetting=LoadInt(appkey, "QuerySetting", m_pref_QuerySetting);
	m_server->SetQuerySetting(m_pref_QuerySetting);
	m_pref_QueryTimeout=LoadInt(appkey, "QueryTimeout", m_pref_QueryTimeout);
	m_server->SetQueryTimeout(m_pref_QueryTimeout);
	m_pref_QueryAccept=LoadInt(appkey, "QueryAccept", m_pref_QueryAccept);
	m_server->SetQueryAccept(m_pref_QueryAccept);

	// marscha@2006 - Is AcceptDialog required even if no user is logged on
	m_pref_QueryIfNoLogon=LoadInt(appkey, "QueryIfNoLogon", m_pref_QueryIfNoLogon);
	m_server->SetQueryIfNoLogon(m_pref_QueryIfNoLogon);

	// Load the password
	LoadPassword(appkey, m_pref_passwd);
	LoadPassword2(appkey, m_pref_passwd2); //PGM

	// Remote access prefs
	m_pref_EnableRemoteInputs=LoadInt(appkey, "InputsEnabled", m_pref_EnableRemoteInputs);
	m_pref_LockSettings=LoadInt(appkey, "LockSetting", m_pref_LockSettings);
	m_pref_DisableLocalInputs=LoadInt(appkey, "LocalInputsDisabled", m_pref_DisableLocalInputs);
	m_pref_EnableJapInput=LoadInt(appkey, "EnableJapInput", m_pref_EnableJapInput);
	m_pref_clearconsole=LoadInt(appkey, "clearconsole", m_pref_clearconsole);
}

void
vncProperties::ApplyUserPrefs()
{
	// APPLY THE CACHED PREFERENCES TO THE SERVER

	// Modif sf@2002
	m_server->EnableFileTransfer(m_pref_EnableFileTransfer);
	m_server->FTUserImpersonation(m_pref_FTUserImpersonation); // sf@2005
	m_server->CaptureAlphaBlending(m_pref_CaptureAlphaBlending); // sf@2005
	m_server->BlackAlphaBlending(m_pref_BlackAlphaBlending); // sf@2005
	m_server->Primary(m_pref_Primary);
	m_server->Secondary(m_pref_Secondary);

	m_server->BlankMonitorEnabled(m_pref_EnableBlankMonitor);
	m_server->BlankInputsOnly(m_pref_BlankInputsOnly); //PGM
	m_server->SetDefaultScale(m_pref_DefaultScale);

	// Update the connection querying settings
	m_server->SetQuerySetting(m_pref_QuerySetting);
	m_server->SetQueryTimeout(m_pref_QueryTimeout);
	m_server->SetQueryAccept(m_pref_QueryAccept);
	m_server->SetAutoIdleDisconnectTimeout(m_pref_IdleTimeout);
	m_server->EnableRemoveWallpaper(m_pref_RemoveWallpaper);
	// adzm - 2010-07 - Disable more effects or font smoothing
	m_server->EnableRemoveFontSmoothing(m_pref_RemoveFontSmoothing);
	m_server->EnableRemoveEffects(m_pref_RemoveEffects);
	m_server->EnableRemoveAero(m_pref_RemoveAero);

	// Is the listening socket closing?

	if (!m_pref_SockConnect)
		m_server->SockConnect(m_pref_SockConnect);

	m_server->EnableHTTPConnect(m_pref_HTTPConnect);
	m_server->EnableXDMCPConnect(m_pref_XDMCPConnect);

	// Are inputs being disabled?
	if (!m_pref_EnableRemoteInputs)
		m_server->EnableRemoteInputs(m_pref_EnableRemoteInputs);
	if (m_pref_DisableLocalInputs)
		m_server->DisableLocalInputs(m_pref_DisableLocalInputs);
	if (m_pref_EnableJapInput)
		m_server->EnableJapInput(m_pref_EnableJapInput);
	m_server->Clearconsole(m_pref_clearconsole);

	// Update the password
	m_server->SetPassword(m_pref_passwd);
	m_server->SetPassword2(m_pref_passwd2); //PGM

	// Now change the listening port settings
	m_server->SetAutoPortSelect(m_pref_AutoPortSelect);
	if (!m_pref_AutoPortSelect)
		// m_server->SetPort(m_pref_PortNumber);
		m_server->SetPorts(m_pref_PortNumber, m_pref_HttpPortNumber); // Tight 1.2.7

	m_server->SockConnect(m_pref_SockConnect);


	// Remote access prefs
	m_server->EnableRemoteInputs(m_pref_EnableRemoteInputs);
	m_server->SetLockSettings(m_pref_LockSettings);
	m_server->DisableLocalInputs(m_pref_DisableLocalInputs);
	m_server->EnableJapInput(m_pref_EnableJapInput);
	m_server->Clearconsole(m_pref_clearconsole);

	// DSM Plugin prefs
	m_server->EnableDSMPlugin(m_pref_UseDSMPlugin);
	m_server->SetDSMPluginName(m_pref_szDSMPlugin);
	
	//adzm 2010-05-12 - dsmplugin config
	m_server->SetDSMPluginConfig(m_pref_DSMPluginConfig);

	if (m_server->IsDSMPluginEnabled()) 
	{
		vnclog.Print(LL_INTINFO, VNCLOG("$$$$$$$$$$ ApplyUserPrefs - Plugin Enabled - Call SetDSMPlugin() \n"));
		m_server->SetDSMPlugin(false);
	}
	else
	{
		vnclog.Print(LL_INTINFO, VNCLOG("$$$$$$$$$$ ApplyUserPrefs - Plugin NOT enabled \n"));
	}

}

void
vncProperties::SaveInt(HKEY key, LPCSTR valname, LONG val)
{
	RegSetValueEx(key, valname, 0, REG_DWORD, (LPBYTE) &val, sizeof(val));
}

void
vncProperties::SavePassword(HKEY key, char *buffer)
{
	RegSetValueEx(key, "Password", 0, REG_BINARY, (LPBYTE) buffer, MAXPWLEN);
}
void //PGM
vncProperties::SavePassword2(HKEY key, char *buffer) //PGM
{ //PGM
	RegSetValueEx(key, "Password2", 0, REG_BINARY, (LPBYTE) buffer, MAXPWLEN); //PGM
} //PGM
void
vncProperties::SaveString(HKEY key,LPCSTR valname, const char *buffer)
{
	RegSetValueEx(key, valname, 0, REG_BINARY, (LPBYTE) buffer, strlen(buffer)+1);
}

void
vncProperties::SaveDSMPluginName(HKEY key, char *buffer)
{
	RegSetValueEx(key, "DSMPlugin", 0, REG_BINARY, (LPBYTE) buffer, MAXPATH);
}

void
vncProperties::LoadDSMPluginName(HKEY key, char *buffer)
{
	DWORD type = REG_BINARY;
	int slen=MAXPATH;
	char inouttext[MAXPATH];

	if (RegQueryValueEx(key,
		"DSMPlugin",
		NULL,
		&type,
		(LPBYTE) &inouttext,
		(LPDWORD) &slen) != ERROR_SUCCESS)
		return;

	if (slen > MAXPATH)
		return;

	memcpy(buffer, inouttext, MAXPATH);
}

void
vncProperties::Save()
{
	HKEY appkey;
	DWORD dw;

	if (!m_allowproperties)
		return;

	// NEW (R3) PREFERENCES ALGORITHM
	// The user's prefs are only saved if the user is allowed to override
	// the machine-local settings specified for them.  Otherwise, the
	// properties entry on the tray icon menu will be greyed out.

	// GET THE CORRECT KEY TO READ FROM

	// Have we loaded user settings, or system settings?
	if (m_usersettings) {
		// Verify that we know who is logged on
		char username[UNLEN+1];
		if (!vncService::CurrentUser((char *)&username, sizeof(username)))
			return;
		if (strcmp(username, "") == 0)
			return;

		// Try to get the per-user, global registry key for WinVNC
		if (RegCreateKeyEx(HKEY_CURRENT_USER,
			WINVNC_REGISTRY_KEY,
			0, REG_NONE, REG_OPTION_NON_VOLATILE,
			KEY_WRITE | KEY_READ, NULL, &appkey, &dw) != ERROR_SUCCESS)
			return;
	} else {
		// Try to get the default local registry key for WinVNC
		HKEY hkLocal;
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			WINVNC_REGISTRY_KEY,
			0, REG_NONE, REG_OPTION_NON_VOLATILE,
			KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS) {
			MessageBoxSecure(NULL, sz_ID_MB1, sz_ID_WVNC, MB_OK);
			return;
		}

		if (RegCreateKeyEx(hkLocal,
			"Default",
			0, REG_NONE, REG_OPTION_NON_VOLATILE,
			KEY_WRITE | KEY_READ, NULL, &appkey, &dw) != ERROR_SUCCESS) {
			RegCloseKey(hkLocal);
			return;
		}
		RegCloseKey(hkLocal);
	}

	// SAVE PER-USER PREFS IF ALLOWED
	SaveUserPrefs(appkey);
	RegCloseKey(appkey);
	RegCloseKey(HKEY_CURRENT_USER);

	// Machine Preferences
	// Get the machine registry key for WinVNC
	HKEY hkLocal,hkDefault;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		WINVNC_REGISTRY_KEY,
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_WRITE | KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
		return;
	if (RegCreateKeyEx(hkLocal,
		"Default",
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_WRITE | KEY_READ,
		NULL,
		&hkDefault,
		&dw) != ERROR_SUCCESS)
		hkDefault = NULL;
	// sf@2003
	SaveInt(hkLocal, "DebugMode", vnclog.GetMode());
	SaveInt(hkLocal, "Avilog", vnclog.GetVideo());
	SaveString(hkLocal, "path", vnclog.GetPath());
	SaveInt(hkLocal, "DebugLevel", vnclog.GetLevel());
	SaveInt(hkLocal, "AllowLoopback", m_server->LoopbackOk());
	SaveInt(hkLocal, "LoopbackOnly", m_server->LoopbackOnly());
	if (hkDefault) SaveInt(hkDefault, "AllowShutdown", m_allowshutdown);
	if (hkDefault) SaveInt(hkDefault, "AllowProperties",  m_allowproperties);
	if (hkDefault) SaveInt(hkDefault, "AllowEditClients", m_alloweditclients);

	SaveInt(hkLocal, "DisableTrayIcon", m_server->GetDisableTrayIcon());
	SaveInt(hkLocal, "MSLogonRequired", m_server->MSLogonRequired());
	// Marscha@2004 - authSSP: save "New MS-Logon" state
	SaveInt(hkLocal, "NewMSLogon", m_server->GetNewMSLogon());
	// sf@2003 - DSM params here
	SaveInt(hkLocal, "UseDSMPlugin", m_server->IsDSMPluginEnabled());
	SaveInt(hkLocal, "ConnectPriority", m_server->ConnectPriority());
	SaveDSMPluginName(hkLocal, m_server->GetDSMPluginName());	
	
	//adzm 2010-05-12 - dsmplugin config
	SaveString(hkLocal, "DSMPluginConfig", m_server->GetDSMPluginConfig());

	if (hkDefault) RegCloseKey(hkDefault);
	if (hkLocal) RegCloseKey(hkLocal);
}

void
vncProperties::SaveUserPrefs(HKEY appkey)
{
	// SAVE THE PER USER PREFS
	vnclog.Print(LL_INTINFO, VNCLOG("saving current settings to registry\n"));

	// Modif sf@2002
	SaveInt(appkey, "FileTransferEnabled", m_server->FileTransferEnabled());
	SaveInt(appkey, "FTUserImpersonation", m_server->FTUserImpersonation()); // sf@2005
	SaveInt(appkey, "BlankMonitorEnabled", m_server->BlankMonitorEnabled());
	SaveInt(appkey, "BlankInputsOnly", m_server->BlankInputsOnly()); //PGM
	SaveInt(appkey, "CaptureAlphaBlending", m_server->CaptureAlphaBlending()); // sf@2005
	SaveInt(appkey, "BlackAlphaBlending", m_server->BlackAlphaBlending()); // sf@2005
	SaveInt(appkey, "primary", m_server->Primary());
	SaveInt(appkey, "secondary", m_server->Secondary());

	SaveInt(appkey, "DefaultScale", m_server->GetDefaultScale());

	SaveInt(appkey, "UseDSMPlugin", m_server->IsDSMPluginEnabled());
	SaveDSMPluginName(appkey, m_server->GetDSMPluginName());
	//adzm 2010-05-12 - dsmplugin config
	SaveString(appkey, "DSMPluginConfig", m_server->GetDSMPluginConfig());

	// Connection prefs
	SaveInt(appkey, "SocketConnect", m_server->SockConnected());
	SaveInt(appkey, "HTTPConnect", m_server->HTTPConnectEnabled());
	SaveInt(appkey, "XDMCPConnect", m_server->XDMCPConnectEnabled());
	SaveInt(appkey, "AutoPortSelect", m_server->AutoPortSelect());
	if (!m_server->AutoPortSelect()) {
		SaveInt(appkey, "PortNumber", m_server->GetPort());
		SaveInt(appkey, "HTTPPortNumber", m_server->GetHttpPort());
	}
	SaveInt(appkey, "InputsEnabled", m_server->RemoteInputsEnabled());
	SaveInt(appkey, "LocalInputsDisabled", m_server->LocalInputsDisabled());
	SaveInt(appkey, "IdleTimeout", m_server->AutoIdleDisconnectTimeout());
	SaveInt(appkey, "EnableJapInput", m_server->JapInputEnabled());

	// Connection querying settings
	SaveInt(appkey, "QuerySetting", m_server->QuerySetting());
	SaveInt(appkey, "QueryTimeout", m_server->QueryTimeout());
	SaveInt(appkey, "QueryAccept", m_server->QueryAccept());

	// Lock settings
	SaveInt(appkey, "LockSetting", m_server->LockSettings());

	// Wallpaper removal
	SaveInt(appkey, "RemoveWallpaper", m_server->RemoveWallpaperEnabled());
	// UI Effects
	// adzm - 2010-07 - Disable more effects or font smoothing
	SaveInt(appkey, "RemoveEffects", m_server->RemoveEffectsEnabled());
	SaveInt(appkey, "RemoveFontSmoothing", m_server->RemoveFontSmoothingEnabled());
	// Composit desktop removal
	SaveInt(appkey, "RemoveAero", m_server->RemoveAeroEnabled());

	// Save the password
	char passwd[MAXPWLEN];
	m_server->GetPassword(passwd);
	SavePassword(appkey, passwd);
	memset(passwd, '\0', MAXPWLEN); //PGM
	m_server->GetPassword2(passwd); //PGM
	SavePassword2(appkey, passwd); //PGM
}


// ********************************************************************
// Ini file part - Wwill replace registry access completely, some day
// WARNING: until then, when adding/modifying a config parameter
//          don't forget to modify both ini file & registry parts !
// ********************************************************************

void vncProperties::LoadFromIniFile()
{
	//if (m_dlgvisible)
	//{
	//	vnclog.Print(LL_INTWARN, VNCLOG("service helper invoked while Properties panel displayed\n"));
	//	return;
	//}

	char username[UNLEN+1];

	// Get the user name / service name
	if (!vncService::CurrentUser((char *)&username, sizeof(username)))
	{
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - NO current user\n"));
		return;
	}

	// If there is no user logged on them default to SYSTEM
	if (strcmp(username, "") == 0)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Force USER SYSTEM 2\n"));
		strcpy((char *)&username, "SYSTEM");
	}

	// Logging/debugging prefs
	vnclog.SetMode(myIniFile.ReadInt("admin", "DebugMode", 0));
	char temp[512];
	myIniFile.ReadString("admin", "path", temp,512);
	vnclog.SetPath(temp);
	vnclog.SetLevel(myIniFile.ReadInt("admin", "DebugLevel", 0));
	vnclog.SetVideo(myIniFile.ReadInt("admin", "Avilog", 0) ? true : false);

	// Disable Tray Icon
	m_server->SetDisableTrayIcon(myIniFile.ReadInt("admin", "DisableTrayIcon", false));

	// Authentication required, loopback allowed, loopbackOnly

	m_server->SetLoopbackOnly(myIniFile.ReadInt("admin", "LoopbackOnly", false));

	m_pref_RequireMSLogon=false;
	m_pref_RequireMSLogon = myIniFile.ReadInt("admin", "MSLogonRequired", m_pref_RequireMSLogon);
	m_server->RequireMSLogon(m_pref_RequireMSLogon);

	// Marscha@2004 - authSSP: added NewMSLogon checkbox to admin props page
	m_pref_NewMSLogon = false;
	m_pref_NewMSLogon = myIniFile.ReadInt("admin", "NewMSLogon", m_pref_NewMSLogon);
	m_server->SetNewMSLogon(m_pref_NewMSLogon);

	// sf@2003 - Moved DSM params here
	m_pref_UseDSMPlugin=false;
	m_pref_UseDSMPlugin = myIniFile.ReadInt("admin", "UseDSMPlugin", m_pref_UseDSMPlugin);
	myIniFile.ReadString("admin", "DSMPlugin",m_pref_szDSMPlugin,128);
	
	//adzm 2010-05-12 - dsmplugin config
	myIniFile.ReadString("admin", "DSMPluginConfig", m_pref_DSMPluginConfig, 512);

	if (m_server->LoopbackOnly()) m_server->SetLoopbackOk(true);
	else m_server->SetLoopbackOk(myIniFile.ReadInt("admin", "AllowLoopback", false));
	m_server->SetAuthRequired(myIniFile.ReadInt("admin", "AuthRequired", true));

	m_server->SetConnectPriority(myIniFile.ReadInt("admin", "ConnectPriority", 0));
	if (!m_server->LoopbackOnly())
	{
		char *authhosts=new char[150];
		myIniFile.ReadString("admin", "AuthHosts",authhosts,150);
		if (authhosts != 0) {
			m_server->SetAuthHosts(authhosts);
			delete [] authhosts;
		} else {
			m_server->SetAuthHosts(0);
		}
	} else {
		m_server->SetAuthHosts(0);
	}

	vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Load User Preferences\n"));

	// Set the default user prefs
	vnclog.Print(LL_INTINFO, VNCLOG("clearing user settings\n"));
	m_pref_AutoPortSelect=TRUE;
    m_pref_HTTPConnect = TRUE;
	m_pref_XDMCPConnect = TRUE;
	m_pref_PortNumber = RFB_PORT_OFFSET; 
	m_pref_SockConnect=TRUE;
	{
	    vncPasswd::FromClear crypt;
	    memcpy(m_pref_passwd, crypt, MAXPWLEN);
	}
	m_pref_QuerySetting=2;
	m_pref_QueryTimeout=10;
	m_pref_QueryAccept=0;
	m_pref_IdleTimeout=0;
	m_pref_EnableRemoteInputs=TRUE;
	m_pref_DisableLocalInputs=FALSE;
	m_pref_EnableJapInput=FALSE;
	m_pref_clearconsole=FALSE;
	m_pref_LockSettings=-1;

	m_pref_RemoveWallpaper=TRUE;
	// adzm - 2010-07 - Disable more effects or font smoothing
	m_pref_RemoveEffects=FALSE;
	m_pref_RemoveFontSmoothing=FALSE;
	m_pref_RemoveAero=TRUE;
    m_alloweditclients = TRUE;
	m_allowshutdown = TRUE;
	m_allowproperties = TRUE;

	// Modif sf@2002
	m_pref_SingleWindow = FALSE;
	m_pref_UseDSMPlugin = FALSE;
	*m_pref_szDSMPlugin = '\0';
	m_pref_DSMPluginConfig[0] = '\0';

	m_pref_EnableFileTransfer = TRUE;
	m_pref_FTUserImpersonation = TRUE;
	m_pref_EnableBlankMonitor = TRUE;
	m_pref_BlankInputsOnly = FALSE;
	m_pref_QueryIfNoLogon = FALSE;
	m_pref_DefaultScale = 1;
	m_pref_CaptureAlphaBlending = FALSE; 
	m_pref_BlackAlphaBlending = FALSE; 

	LoadUserPrefsFromIniFile();
	m_allowshutdown = myIniFile.ReadInt("admin", "AllowShutdown", m_allowshutdown);
	m_allowproperties = myIniFile.ReadInt("admin", "AllowProperties", m_allowproperties);
	m_alloweditclients = myIniFile.ReadInt("admin", "AllowEditClients", m_alloweditclients);

    m_ftTimeout = myIniFile.ReadInt("admin", "FileTransferTimeout", m_ftTimeout);
    if (m_ftTimeout > 60)
        m_ftTimeout = 60;

    m_keepAliveInterval = myIniFile.ReadInt("admin", "KeepAliveInterval", m_keepAliveInterval);
    if (m_keepAliveInterval >= (m_ftTimeout - KEEPALIVE_HEADROOM))
        m_keepAliveInterval = m_ftTimeout - KEEPALIVE_HEADROOM;

	// adzm 2010-08
	m_socketKeepAliveTimeout = myIniFile.ReadInt("admin", "SocketKeepAliveTimeout", m_socketKeepAliveTimeout); 
	if (m_socketKeepAliveTimeout < 0) m_socketKeepAliveTimeout = 0;

    m_server->SetFTTimeout(m_ftTimeout);
    m_server->SetKeepAliveInterval(m_keepAliveInterval);
	m_server->SetSocketKeepAliveTimeout(m_socketKeepAliveTimeout); // adzm 2010-08
    

	ApplyUserPrefs();
}


void vncProperties::LoadUserPrefsFromIniFile()
{
	// Modif sf@2002
	m_pref_EnableFileTransfer = myIniFile.ReadInt("admin", "FileTransferEnabled", m_pref_EnableFileTransfer);
	m_pref_FTUserImpersonation = myIniFile.ReadInt("admin", "FTUserImpersonation", m_pref_FTUserImpersonation); // sf@2005
	m_pref_EnableBlankMonitor = myIniFile.ReadInt("admin", "BlankMonitorEnabled", m_pref_EnableBlankMonitor);
	m_pref_BlankInputsOnly = myIniFile.ReadInt("admin", "BlankInputsOnly", m_pref_BlankInputsOnly); //PGM
	m_pref_DefaultScale = myIniFile.ReadInt("admin", "DefaultScale", m_pref_DefaultScale);
	m_pref_CaptureAlphaBlending = myIniFile.ReadInt("admin", "CaptureAlphaBlending", m_pref_CaptureAlphaBlending); // sf@2005
	m_pref_BlackAlphaBlending = myIniFile.ReadInt("admin", "BlackAlphaBlending", m_pref_BlackAlphaBlending); // sf@2005

	m_pref_UseDSMPlugin = myIniFile.ReadInt("admin", "UseDSMPlugin", m_pref_UseDSMPlugin);
	myIniFile.ReadString("admin", "DSMPlugin",m_pref_szDSMPlugin,128);
	
	//adzm 2010-05-12 - dsmplugin config
	myIniFile.ReadString("admin", "DSMPluginConfig", m_pref_DSMPluginConfig, 512);
	
	m_pref_Primary = myIniFile.ReadInt("admin", "primary", m_pref_Primary);
	m_pref_Secondary = myIniFile.ReadInt("admin", "secondary", m_pref_Secondary);

	// Connection prefs
	m_pref_SockConnect=myIniFile.ReadInt("admin", "SocketConnect", m_pref_SockConnect);
	m_pref_HTTPConnect=myIniFile.ReadInt("admin", "HTTPConnect", m_pref_HTTPConnect);
	m_pref_XDMCPConnect=myIniFile.ReadInt("admin", "XDMCPConnect", m_pref_XDMCPConnect);
	m_pref_AutoPortSelect=myIniFile.ReadInt("admin", "AutoPortSelect", m_pref_AutoPortSelect);
	m_pref_PortNumber=myIniFile.ReadInt("admin", "PortNumber", m_pref_PortNumber);
	m_pref_HttpPortNumber=myIniFile.ReadInt("admin", "HTTPPortNumber",
									DISPLAY_TO_HPORT(PORT_TO_DISPLAY(m_pref_PortNumber)));
	m_pref_IdleTimeout=myIniFile.ReadInt("admin", "IdleTimeout", m_pref_IdleTimeout);
	
	m_pref_RemoveWallpaper=myIniFile.ReadInt("admin", "RemoveWallpaper", m_pref_RemoveWallpaper);
	// adzm - 2010-07 - Disable more effects or font smoothing
	m_pref_RemoveEffects=myIniFile.ReadInt("admin", "RemoveEffects", m_pref_RemoveEffects);
	m_pref_RemoveFontSmoothing=myIniFile.ReadInt("admin", "RemoveFontSmoothing", m_pref_RemoveFontSmoothing);
	m_pref_RemoveAero=myIniFile.ReadInt("admin", "RemoveAero", m_pref_RemoveAero);

	// Connection querying settings
	m_pref_QuerySetting=myIniFile.ReadInt("admin", "QuerySetting", m_pref_QuerySetting);
	m_server->SetQuerySetting(m_pref_QuerySetting);
	m_pref_QueryTimeout=myIniFile.ReadInt("admin", "QueryTimeout", m_pref_QueryTimeout);
	m_server->SetQueryTimeout(m_pref_QueryTimeout);
	m_pref_QueryAccept=myIniFile.ReadInt("admin", "QueryAccept", m_pref_QueryAccept);
	m_server->SetQueryAccept(m_pref_QueryAccept);

	// marscha@2006 - Is AcceptDialog required even if no user is logged on
	m_pref_QueryIfNoLogon=myIniFile.ReadInt("admin", "QueryIfNoLogon", m_pref_QueryIfNoLogon);
	m_server->SetQueryIfNoLogon(m_pref_QueryIfNoLogon);

	// Load the password
	myIniFile.ReadPassword(m_pref_passwd,MAXPWLEN);
	myIniFile.ReadPassword2(m_pref_passwd2,MAXPWLEN); //PGM

	// Remote access prefs
	m_pref_EnableRemoteInputs=myIniFile.ReadInt("admin", "InputsEnabled", m_pref_EnableRemoteInputs);
	m_pref_LockSettings=myIniFile.ReadInt("admin", "LockSetting", m_pref_LockSettings);
	m_pref_DisableLocalInputs=myIniFile.ReadInt("admin", "LocalInputsDisabled", m_pref_DisableLocalInputs);
	m_pref_EnableJapInput=myIniFile.ReadInt("admin", "EnableJapInput", m_pref_EnableJapInput);
	m_pref_clearconsole=myIniFile.ReadInt("admin", "clearconsole", m_pref_clearconsole);
	G_SENDBUFFER=myIniFile.ReadInt("admin", "sendbuffer", G_SENDBUFFER);
}


void vncProperties::SaveToIniFile()
{
	if (!m_allowproperties)
		return;

	// SAVE PER-USER PREFS IF ALLOWED
	bool use_uac=false;
	if (!myIniFile.IsWritable()  || vncService::RunningAsService())
			{
				// We can't write to the ini file , Vista in service mode
				if (!Copy_to_Temp( m_Tempfile)) return;
				myIniFile.IniFileSetTemp( m_Tempfile);
				use_uac=true;
			}

	SaveUserPrefsToIniFile();
	myIniFile.WriteInt("admin", "DebugMode", vnclog.GetMode());
	myIniFile.WriteInt("admin", "Avilog", vnclog.GetVideo());
	myIniFile.WriteString("admin", "path", vnclog.GetPath());
	myIniFile.WriteInt("admin", "DebugLevel", vnclog.GetLevel());
	myIniFile.WriteInt("admin", "AllowLoopback", m_server->LoopbackOk());
	myIniFile.WriteInt("admin", "LoopbackOnly", m_server->LoopbackOnly());
	myIniFile.WriteInt("admin", "AllowShutdown", m_allowshutdown);
	myIniFile.WriteInt("admin", "AllowProperties",  m_allowproperties);
	myIniFile.WriteInt("admin", "AllowEditClients", m_alloweditclients);
    myIniFile.WriteInt("admin", "FileTransferTimeout", m_ftTimeout);
    myIniFile.WriteInt("admin", "KeepAliveInterval", m_keepAliveInterval);
	// adzm 2010-08
    myIniFile.WriteInt("admin", "SocketKeepAliveTimeout", m_socketKeepAliveTimeout);

	myIniFile.WriteInt("admin", "DisableTrayIcon", m_server->GetDisableTrayIcon());
	myIniFile.WriteInt("admin", "MSLogonRequired", m_server->MSLogonRequired());
	// Marscha@2004 - authSSP: save "New MS-Logon" state
	myIniFile.WriteInt("admin", "NewMSLogon", m_server->GetNewMSLogon());
	// sf@2003 - DSM params here
	myIniFile.WriteInt("admin", "UseDSMPlugin", m_server->IsDSMPluginEnabled());
	myIniFile.WriteInt("admin", "ConnectPriority", m_server->ConnectPriority());
	myIniFile.WriteString("admin", "DSMPlugin",m_server->GetDSMPluginName());

	//adzm 2010-05-12 - dsmplugin config
	myIniFile.WriteString("admin", "DSMPluginConfig", m_server->GetDSMPluginConfig());

	if (use_uac==true)
	{
	myIniFile.copy_to_secure();
	myIniFile.IniFileSetSecure();
	}
}


void vncProperties::SaveUserPrefsToIniFile()
{
	// SAVE THE PER USER PREFS
	vnclog.Print(LL_INTINFO, VNCLOG("saving current settings to registry\n"));

	// Modif sf@2002
	myIniFile.WriteInt("admin", "FileTransferEnabled", m_server->FileTransferEnabled());
	myIniFile.WriteInt("admin", "FTUserImpersonation", m_server->FTUserImpersonation()); // sf@2005
	myIniFile.WriteInt("admin", "BlankMonitorEnabled", m_server->BlankMonitorEnabled());
	myIniFile.WriteInt("admin", "BlankInputsOnly", m_server->BlankInputsOnly()); //PGM
	myIniFile.WriteInt("admin", "CaptureAlphaBlending", m_server->CaptureAlphaBlending()); // sf@2005
	myIniFile.WriteInt("admin", "BlackAlphaBlending", m_server->BlackAlphaBlending()); // sf@2005

	myIniFile.WriteInt("admin", "DefaultScale", m_server->GetDefaultScale());

	myIniFile.WriteInt("admin", "UseDSMPlugin", m_server->IsDSMPluginEnabled());
	myIniFile.WriteString("admin", "DSMPlugin",m_server->GetDSMPluginName());

	//adzm 2010-05-12 - dsmplugin config
	myIniFile.WriteString("admin", "DSMPluginConfig", m_server->GetDSMPluginConfig());

	myIniFile.WriteInt("admin", "primary", m_server->Primary());
	myIniFile.WriteInt("admin", "secondary", m_server->Secondary());

	// Connection prefs
	myIniFile.WriteInt("admin", "SocketConnect", m_server->SockConnected());
	myIniFile.WriteInt("admin", "HTTPConnect", m_server->HTTPConnectEnabled());
	myIniFile.WriteInt("admin", "XDMCPConnect", m_server->XDMCPConnectEnabled());
	myIniFile.WriteInt("admin", "AutoPortSelect", m_server->AutoPortSelect());
	if (!m_server->AutoPortSelect()) {
		myIniFile.WriteInt("admin", "PortNumber", m_server->GetPort());
		myIniFile.WriteInt("admin", "HTTPPortNumber", m_server->GetHttpPort());
	}
	myIniFile.WriteInt("admin", "InputsEnabled", m_server->RemoteInputsEnabled());
	myIniFile.WriteInt("admin", "LocalInputsDisabled", m_server->LocalInputsDisabled());
	myIniFile.WriteInt("admin", "IdleTimeout", m_server->AutoIdleDisconnectTimeout());
	myIniFile.WriteInt("admin", "EnableJapInput", m_server->JapInputEnabled());

	// Connection querying settings
	myIniFile.WriteInt("admin", "QuerySetting", m_server->QuerySetting());
	myIniFile.WriteInt("admin", "QueryTimeout", m_server->QueryTimeout());
	myIniFile.WriteInt("admin", "QueryAccept", m_server->QueryAccept());

	// Lock settings
	myIniFile.WriteInt("admin", "LockSetting", m_server->LockSettings());

	// Wallpaper removal
	myIniFile.WriteInt("admin", "RemoveWallpaper", m_server->RemoveWallpaperEnabled());
	// UI Effects
	// adzm - 2010-07 - Disable more effects or font smoothing
	myIniFile.WriteInt("admin", "RemoveEffects", m_server->RemoveEffectsEnabled());
	myIniFile.WriteInt("admin", "RemoveFontSmoothing", m_server->RemoveFontSmoothingEnabled());
	// Composit desktop removal
	myIniFile.WriteInt("admin", "RemoveAero", m_server->RemoveAeroEnabled());

	// Save the password
	char passwd[MAXPWLEN];
	m_server->GetPassword(passwd);
	myIniFile.WritePassword(passwd);
	memset(passwd, '\0', MAXPWLEN); //PGM
	m_server->GetPassword2(passwd); //PGM
	myIniFile.WritePassword2(passwd); //PGM
}


void vncProperties::ReloadDynamicSettings()
{
	char username[UNLEN+1];

	// Get the user name / service name
	if (!vncService::CurrentUser((char *)&username, sizeof(username)))
	{
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - NO current user\n"));
		return;
	}

	// If there is no user logged on them default to SYSTEM
	if (strcmp(username, "") == 0)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Force USER SYSTEM 2\n"));
		strcpy((char *)&username, "SYSTEM");
	}

	// Logging/debugging prefs
	vnclog.SetMode(myIniFile.ReadInt("admin", "DebugMode", 0));
	vnclog.SetLevel(myIniFile.ReadInt("admin", "DebugLevel", 0));
}
