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
class vncPropertiesPoll;

#if (!defined(_WINVNC_VNCPROPERTIESPOLL))
#define _WINVNC_VNCPROPERTIESPOLL

// Includes
#include "stdhdrs.h"
#include "vncserver.h"
#include "inifile.h"
#include <userenv.h>
// The vncPropertiesPoll class itself
class vncPropertiesPoll
{
public:
	// Constructor/destructor
	vncPropertiesPoll();
	~vncPropertiesPoll();

	// Initialisation
	BOOL Init(vncServer *server);

	// The dialog box window proc
	static BOOL CALLBACK DialogProcPoll(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Display the properties dialog
	// If usersettings is TRUE then the per-user settings come up
	// If usersettings is FALSE then the default system settings come up
	void Show(BOOL show, BOOL usersettings);

	// Loading & saving of preferences
	void Load(BOOL usersettings);
	void ResetRegistry();

	void Save();

	BOOL m_fUseRegistry;
	// Ini file
	IniFile myIniFile;
	void LoadFromIniFile();
	void LoadUserPrefsPollFromIniFile();
	void SaveToIniFile();
	void SaveUserPrefsPollToIniFile();

	// Implementation
protected:
	// The server object to which this properties object is attached.
	vncServer *			m_server;

	// Flag to indicate whether the currently loaded settings are for
	// the current user, or are default system settings
	BOOL				m_usersettings;


	// String handling
	char * LoadString(HKEY k, LPCSTR valname);
	void SaveString(HKEY k, LPCSTR valname, const char *buffer);

	// Manipulate the registry settings
	LONG LoadInt(HKEY key, LPCSTR valname, LONG defval);
	void SaveInt(HKEY key, LPCSTR valname, LONG val);


	// Loading/saving all the user prefs
	void LoadUserPrefsPoll(HKEY appkey);
	void SaveUserPrefsPoll(HKEY appkey);

	// [v1.0.2-jp2 fix]
	void LoadSingleWindowName(HKEY key, char *buffer);

	// Making the loaded user prefs active
	void ApplyUserPrefs();
	
	BOOL m_returncode_valid;
	BOOL m_dlgvisible;

	BOOL m_pref_TurboMode;
	
	BOOL m_pref_PollUnderCursor;
	BOOL m_pref_PollForeground;
	BOOL m_pref_PollFullScreen;
	BOOL m_pref_PollConsoleOnly;
	BOOL m_pref_PollOnEventOnly;
	LONG m_pref_MaxCpu;

	BOOL m_pref_Driver;
	BOOL m_pref_Hook;
	BOOL m_pref_Virtual;

	// [v1.0.2-jp2 fix]
	BOOL m_pref_SingleWindow;
	char m_pref_szSingleWindowName[32];
	char m_Tempfile[MAX_PATH];

};

#endif // _WINVNC_vncPropertiesPoll
