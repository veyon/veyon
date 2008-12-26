/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004 Martin Scharpf. All Rights Reserved.
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
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
// http://ultravnc.sourceforge.net/
// /macine-vnc Greg Wood (wood@agressiv.com)
//#include "..\..\winvnc\stdhdrs.h"
#include <objbase.h> // for CoInitialize/CoUninitialize ???
#include <time.h>
#include "vncSSP.h"
// From vncAccessControl.h
#define ViewOnly 0x0001
#define Interact 0x0002

#include "..\..\winvnc\localization.h" // Act : add localization on messages

CheckUserPasswordSDFn CheckUserPasswordSD = 0;

const TCHAR REGISTRY_KEY [] = _T("Software\\UltraVnc");

AUTHSSP_API
int CUPSD(const char * userin, const char *password, const char *machine)
{
	DWORD dwAccessGranted = 0;
	BOOL isAccessOK = FALSE;
	BOOL NT4OS=FALSE;
	BOOL W2KOS=FALSE;
	BOOL isAuthenticated = FALSE;
	bool isViewOnly = false;
	bool isInteract = false;
	TCHAR machine2[MAXSTRING];
	TCHAR user2[MAXSTRING];
#if defined(UNICODE) || defined(_UNICODE)
	mbstowcs(machine2, machine, MAXSTRING);
	mbstowcs(user2, userin, MAXSTRING);
#else
	strcpy(machine2, machine);
	strcpy(user2, userin);
#endif

	OSVERSIONINFO VerInfo;
	VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	if (!GetVersionEx (&VerInfo)) {  // If this fails, something has gone wrong
		return FALSE;
	}
	
	if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) { // WinNT 3.51 or better 
		vncAccessControl vncAC;
		isAccessOK = CUPSD2(userin, password, vncAC.GetSD(), &isAuthenticated, &dwAccessGranted);
		// This logging should be moved to LOGLOGONUSER etc.
		FILE *file = fopen("WinVNC-authSSP.log", "a");
		if (file) {
            time_t current;
			time(&current);
			char* timestr = ctime(&current);
			timestr[24] = '\0'; // remove newline
			fprintf(file, "%s - CUPSD2: Access is %u, user %s is %sauthenticated, access granted is 0x%x\n",
				timestr, isAccessOK, userin, isAuthenticated ? "" : "not ", dwAccessGranted);
			fclose(file);
		}
	} else { // message text to be moved to localization.h
		MessageBox(NULL, _T("New MS-Logon currently not supported on Win9x"), _T("Warning"), MB_OK);
		return FALSE;
	}

	if (isAccessOK) {
		if (dwAccessGranted & ViewOnly) isViewOnly = true;
		if (dwAccessGranted & Interact) isInteract = true;
	}
	
	//LookupAccountName(NULL, user2, Sid, cbSid, DomainName, cbDomainName, peUse);

	if (isInteract)	{
		LOG(0x00640001L, _T("Connection received from %s using %s account\n"), machine2, user2);
	} else if (isViewOnly) {
		LOG(0x00640001L, _T("Connection received from %s using %s account\n"), machine2, user2);
		isAccessOK = 2;
	} else {
		LOG(0x00640002L, _T("Invalid attempt (not %s) from client %s using %s account\n"), 
			isAuthenticated ? _T("authorized") : _T("authenticated"), machine2, user2);
	}
	return isAccessOK;
}
	

TCHAR *AddToModuleDir(TCHAR *filename, int length){
	TCHAR *szCurrentDir = new TCHAR[length];
	if (GetModuleFileName(NULL, szCurrentDir, length))
	{
		TCHAR *p = _tcsrchr(szCurrentDir, '\\');
		*p = '\0';
		_tcscat(szCurrentDir,_T("\\"));
		_tcscat(szCurrentDir, filename);
	}
	filename = szCurrentDir;
	return filename;
}