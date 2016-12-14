/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2010 UltraVNC Team Members. All Rights Reserved.
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
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////

class vncProperties;

#if (!defined(_WINVNC_VNCPROPERTIES))
#define _WINVNC_VNCPROPERTIES

// Includes
// Marscha@2004 - authSSP: objbase.h needed for CoInitialize etc.
#include <objbase.h>

#include "stdhdrs.h"
#include "vncserver.h"
#include "vncsetauth.h"
#include "inifile.h"
#include <userenv.h>
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
	int					m_IdleInputTimeout;


	// Password handling
	void LoadPassword(HKEY k, char *buffer);
	void SavePassword(HKEY k, char *buffer);
	void LoadPassword2(HKEY k, char *buffer); //PGM
	void SavePassword2(HKEY k, char *buffer); //PGM

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
	BOOL m_pref_AutoPortSelect;
	LONG m_pref_PortNumber;
	LONG m_pref_HttpPortNumber;  // TightVNC 1.1.8
	char m_pref_passwd[MAXPWLEN];
	char m_pref_passwd2[MAXPWLEN]; //PGM
	UINT m_pref_QuerySetting;
	// Marscha@2006 - Is AcceptDialog required even if no user is logged on
	UINT m_pref_QueryIfNoLogon;
	UINT m_pref_QueryAccept;
	UINT m_pref_QueryTimeout;
	UINT m_pref_IdleTimeout;
	BOOL m_pref_RemoveWallpaper;
	// adzm - 2010-07 - Disable more effects or font smoothing
	BOOL m_pref_RemoveEffects;
	BOOL m_pref_RemoveFontSmoothing;
	BOOL m_pref_RemoveAero;
	BOOL m_pref_EnableRemoteInputs;
	int m_pref_LockSettings;
	BOOL m_pref_DisableLocalInputs;
	BOOL m_pref_EnableJapInput;
	BOOL m_pref_clearconsole;

	// Modif sf@2002
	// [v1.0.2-jp2 fix]
	BOOL m_pref_SingleWindow;
	BOOL m_pref_EnableFileTransfer;
	BOOL m_pref_FTUserImpersonation;
	BOOL m_pref_EnableBlankMonitor;
	BOOL m_pref_BlankInputsOnly; //PGM
	int  m_pref_DefaultScale;
	BOOL m_pref_RequireMSLogon;

	// Marscha@2004 - authSSP: added state of "New MS-Logon"
	BOOL m_pref_NewMSLogon;

	BOOL m_pref_UseDSMPlugin;
	char m_pref_szDSMPlugin[128];
	//adzm 2010-05-12 - dsmplugin config
	char m_pref_DSMPluginConfig[512];

    void LoadDSMPluginName(HKEY key, char *buffer);
	void SaveDSMPluginName(HKEY key, char *buffer); 
	vncSetAuth		m_vncauth;

	char m_pref_path111[500];
	char m_Tempfile[MAX_PATH];
	BOOL m_pref_Primary;
	BOOL m_pref_Secondary;

private:
	void InitPortSettings(HWND hwnd); // TightVNC 1.1.8


};

#endif // _WINVNC_VNCPROPERTIES
