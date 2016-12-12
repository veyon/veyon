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

//#include "..\..\winvnc\localization.h" // Act : add localization on messages

AUTHSSP_API
int CUPSD(const char * userin, const char *password, const char *machine)
{
	BOOL isAuthenticated = FALSE;
	TCHAR machine2[MAXSTRING];
	TCHAR user2[MAXSTRING];
#if defined(UNICODE) || defined(_UNICODE)
	mbstowcs(machine2, machine, MAXSTRING);
	mbstowcs(user2, userin, MAXSTRING);
#else
	strcpy(machine2, machine);
	strcpy(user2, userin);
#endif

	return CUPSD2(userin, password);
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
