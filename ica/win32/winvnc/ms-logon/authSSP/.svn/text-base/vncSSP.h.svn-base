/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004 Martin Scharpf, B. Braun Melsungen AG. All Rights Reserved.
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
//
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the AUTHSSP_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// AUTHSSP_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef AUTHSSP_EXPORTS
#define AUTHSSP_API __declspec(dllexport)
#else
#define AUTHSSP_API __declspec(dllimport)
#endif

#include <tchar.h>
#include "vncAccessControl.h"

typedef BOOL (*CheckUserPasswordSDFn)(const char * domainuser,
									  const char *password,
									  PSECURITY_DESCRIPTOR psdSD,
									  PBOOL isAuthenticated,
									  PDWORD pdwAccessGranted);

#define MAXSTRING 254

AUTHSSP_API int CUPSD(const char * userin, const char *password, const char *machine);
void LOG(long EvenID, const TCHAR *format, ...);
TCHAR * AddToModuleDir(TCHAR *filename, int length);

extern BOOL CUPSD2(const char*userin, const char *password, PSECURITY_DESCRIPTOR psdSD, PBOOL pisAuthenticated, PDWORD pdwAccessGranted);