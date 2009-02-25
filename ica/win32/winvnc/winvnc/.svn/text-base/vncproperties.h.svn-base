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


// vncProperties

// Object implementing the Properties dialog for WinVNC.
// The Properties dialog is displayed whenever the user selects the
// Properties option from the system tray menu.
// The Properties dialog also takes care of loading the program
// settings and saving them on exit.

class vncProperties;

#if (!defined(_WINVNC_VNCPROPERTIES))
#define _WINVNC_VNCPROPERTIES

// Includes
// Marscha@2004 - authSSP: objbase.h needed for CoInitialize etc.
#include <objbase.h>

#include "stdhdrs.h"
#include "vncServer.h"
#include "vncSetAuth.h"
#include "inifile.h"
#include <Userenv.h>
// The vncProperties class itself
class vncProperties
{
public:
	// Constructor/destructor
	vncProperties();
	~vncProperties();

	// Initialisation
	BOOL Init(vncServer *server);

	// The dialog box window proc
	static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Display the properties dialog
	// If usersettings is TRUE then the per-user settings come up
	// If usersettings is FALSE then the default system settings come up
	void ShowAdmin(BOOL show, BOOL usersettings);

	// Loading & saving of preferences
	void Load(BOOL usersettings);
	void ResetRegistry();

	void Save();


	// TRAY ICON MENU SETTINGS
	BOOL AllowProperties() {return m_allowproperties;};
	BOOL AllowShutdown() {return m_allowshutdown;};
	BOOL AllowEditClients() {return m_alloweditclients;};
	bool Lock_service_helper;

	BOOL m_fUseRegistry;
	// Ini file
	IniFile myIniFile;
	void LoadFromIniFile();
	void LoadUserPrefsFromIniFile();
	void SaveToIniFile();
	void SaveUserPrefsToIniFile();
    void ReloadDynamicSettings();

	// Implementation
protected:
	// The server object to which this properties object is attached.
	vncServer *			m_server;

	// Flag to indicate whether the currently loaded settings are for
	// the current user, or are default system settings
	BOOL				m_usersettings;

	// Tray icon menu settings
	BOOL				m_allowproperties;
	BOOL				m_allowshutdown;
	BOOL				m_alloweditclients;
    int                 m_ftTimeout;
    int                 m_keepAliveInterval;


	// Password handling
	void LoadPassword(HKEY k, char *buffer);
	void SavePassword(HKEY k, char *buffer);

	// String handling
	char * LoadString(HKEY k, LPCSTR valname);
	void SaveString(HKEY k, LPCSTR valname, const char *buffer);

	// Manipulate the registry settings
	LONG LoadInt(HKEY key, LPCSTR valname, LONG defval);
	void SaveInt(HKEY key, LPCSTR valname, LONG val);

	// Loading/saving all the user prefs
	void LoadUserPrefs(HKEY appkey);
	void SaveUserPrefs(HKEY appkey);

	// Making the loaded user prefs active
	void ApplyUserPrefs();
	
	BOOL m_returncode_valid;
	BOOL m_dlgvisible;

	// STORAGE FOR THE PROPERTIES PRIOR TO APPLICATION
	BOOL m_pref_SockConnect;
	BOOL m_pref_HTTPConnect;
	BOOL m_pref_XDMCPConnect;
	BOOL m_pref_AutoPortSelect;
	LONG m_pref_PortNumber;
	LONG m_pref_HttpPortNumber;  // TightVNC 1.1.7
	char m_pref_passwd[MAXPWLEN];
	UINT m_pref_QuerySetting;
	// Marscha@2006 - Is AcceptDialog required even if no user is logged on
	UINT m_pref_QueryIfNoLogon;
	UINT m_pref_QueryAccept;
	UINT m_pref_QueryTimeout;
	UINT m_pref_IdleTimeout;
	BOOL m_pref_RemoveWallpaper;
	BOOL m_pref_RemoveAero;
	BOOL m_pref_EnableRemoteInputs;
	int m_pref_LockSettings;
	BOOL m_pref_DisableLocalInputs;
	BOOL m_pref_EnableJapInput;

	// Modif sf@2002
	// [v1.0.2-jp2 fix]
	BOOL m_pref_SingleWindow;
	BOOL m_pref_EnableFileTransfer;
	BOOL m_pref_FTUserImpersonation;
	BOOL m_pref_EnableBlankMonitor;
	int  m_pref_DefaultScale;
	BOOL m_pref_RequireMSLogon;
	BOOL m_pref_CaptureAlphaBlending;
	BOOL m_pref_BlackAlphaBlending;
//	BOOL m_pref_GammaGray;	// [v1.0.2-jp1 fix1]

	
	// Marscha@2004 - authSSP: added state of "New MS-Logon"
	BOOL m_pref_NewMSLogon;

	BOOL m_pref_UseDSMPlugin;
	char m_pref_szDSMPlugin[128];
    void LoadDSMPluginName(HKEY key, char *buffer);
	void SaveDSMPluginName(HKEY key, char *buffer); 
	vncSetAuth		m_vncauth;

	char m_pref_path111[500];
	char m_Tempfile[MAX_PATH];
	BOOL m_pref_Primary;
	BOOL m_pref_Secundary;

private:
	void InitPortSettings(HWND hwnd); // TightVNC 1.1.7


};

#endif // _WINVNC_VNCPROPERTIES
