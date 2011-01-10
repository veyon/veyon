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


// vncSetAuth

// Object implementing the About dialog for WinVNC.
#include "inifile.h"
class vncSetAuth;

#if (!defined(_WINVNC_VNCSETAUTH))
#define _WINVNC_VNCSETAUTH

// Includes
#include "stdhdrs.h"
#include "vncserver.h"

// The vncSetAuth class itself
class vncSetAuth
{
public:
	// Constructor/destructor
	vncSetAuth();
	~vncSetAuth();

	// Initialisation
	BOOL Init(vncServer *server);

	// The dialog box window proc
	static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// General
	void Show(BOOL show);
	void SetTemp(char *TempFile){strcpy_s(m_Tempfile,MAXPATH,TempFile);};
	char m_Tempfile[MAXPATH];

	// Implementation
	BOOL m_dlgvisible;

	void OpenRegistry();
	void CloseRegistry();
	LONG LoadInt(HKEY key, LPCSTR valname, LONG defval);
	TCHAR * LoadString(HKEY key, LPCSTR keyname);
	void SaveInt(HKEY key, LPCSTR valname, LONG val);
	void SaveString(HKEY key,LPCSTR valname, TCHAR *buffer);
	void savegroup1(TCHAR *value);
	TCHAR* Readgroup1();
	void savegroup2(TCHAR *value);
	TCHAR* Readgroup2();
	void savegroup3(TCHAR *value);
	TCHAR* Readgroup3();
	LONG Readlocdom1(LONG returnvalue);
	void savelocdom1(LONG value);
	LONG Readlocdom2(LONG returnvalue);
	void savelocdom2(LONG value);
	LONG Readlocdom3(LONG returnvalue);
	void savelocdom3(LONG value);

	HKEY hkLocal;
	HKEY hkDefault;
	HKEY hkUser;

	char *group1;
	char *group2;
	char *group3;
	long locdom1;
	long locdom2;
	long locdom3;
	char pszgroup1[256];
	char pszgroup2[256];
	char pszgroup3[256];

	vncServer *			m_server;
	BOOL m_fUseRegistry;
	IniFile myIniFile;
};

#endif // _WINVNC_VNCPROPERTIES
