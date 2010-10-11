/////////////////////////////////////////////////////////////////////////////
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
//
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>
int OSTYPE=4;
void
SetOSVersion()
{
	OSVERSIONINFO OSversion;
	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);

	GetVersionEx(&OSversion);

	switch(OSversion.dwPlatformId)
	{
		case VER_PLATFORM_WIN32_NT:
								  if(OSversion.dwMajorVersion==5 && OSversion.dwMinorVersion==0)
									 OSTYPE=1;							    
								  if(OSversion.dwMajorVersion==5 && OSversion.dwMinorVersion==1)
									 OSTYPE=1;
								  if(OSversion.dwMajorVersion==5)
									 OSTYPE=1;
								  if(OSversion.dwMajorVersion>=6) // handle anything >= 6
									 OSTYPE=2;
								  if(OSversion.dwMajorVersion<=4) 	  
								     OSTYPE=3;
								  break;
		case VER_PLATFORM_WIN32_WINDOWS:
								if(OSversion.dwMinorVersion==0) 
								{
									OSTYPE=5; //95
									break;
								}
								OSTYPE=4;
	}
}

int OSversion()
{
	return OSTYPE;
}