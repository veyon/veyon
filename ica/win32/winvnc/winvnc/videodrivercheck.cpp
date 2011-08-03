/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2010 Ultr@VNC Team Members. All Rights Reserved.
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

#include <windows.h>
#include <stdlib.h>
#include "vncOSVersion.h"

DWORD MessageBoxSecure(HWND hWnd,LPCTSTR lpText,LPCTSTR lpCaption,UINT uType);
typedef BOOL (WINAPI* pEnumDisplayDevices)(PVOID,DWORD,PVOID,DWORD);

/*bool CheckDriver2(void)
{
	SC_HANDLE       schMaster;
	SC_HANDLE       schSlave;

	schMaster = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS); 

    schSlave = OpenService (schMaster, "vnccom", SERVICE_ALL_ACCESS);

    if (schSlave == NULL) 
    {
     CloseServiceHandle(schMaster);
	 return false;
    }
	else
	{
		CloseServiceHandle (schSlave);
		CloseServiceHandle(schMaster);
		return true;
	}
}*/


///////////////////////////////////////////////////////////////////
BOOL GetDllProductVersion(char* dllName, char *vBuffer, int size)
{
   char *versionInfo;
   void *lpBuffer;
   unsigned int cBytes;
   DWORD rBuffer;

   if( !dllName || !vBuffer )
      return(FALSE);

//   DWORD sName = GetModuleFileName(NULL, fileName, sizeof(fileName));
// FYI only
    /*char systemroot[150];
	GetEnvironmentVariable("SystemRoot", systemroot, 150);
	char exe_file_name[MAX_PATH];
	strcpy(exe_file_name,systemroot);
	strcat(exe_file_name,"\\system32\\driver\\");
	strcat(exe_file_name,dllName);*/

   DWORD sVersion = GetFileVersionInfoSize(dllName, &rBuffer);
   if (sVersion==0) return (FALSE);
   versionInfo = new char[sVersion];

   BOOL resultValue = VerQueryValue(versionInfo,  

TEXT("\\StringFileInfo\\040904b0\\ProductVersion"), 
                                    &lpBuffer, 
                                    &cBytes);

   if( !resultValue )
   {
	   resultValue = VerQueryValue(versionInfo,TEXT("\\StringFileInfo\\000004b0\\ProductVersion"), 
                                    &lpBuffer, 
                                    &cBytes);

   }

   if( resultValue )
   {
      strncpy(vBuffer, (char *) lpBuffer, size);
      delete []versionInfo;
      return(TRUE);
   }
   else
   {
      *vBuffer = '\0';
      delete []versionInfo;
      return(FALSE);
   }
}

///////////////////////////////////////////////////////////////////

bool
CheckVideoDriver(bool Box)
{
		typedef BOOL (WINAPI* pEnumDisplayDevices)(PVOID,DWORD,PVOID,DWORD);
		pEnumDisplayDevices pd=NULL;
		LPSTR driverName = "mv video hook driver2";
		BOOL DriverFound;
		DEVMODE devmode;
		FillMemory(&devmode, sizeof(DEVMODE), 0);
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmDriverExtra = 0;
		/*BOOL change = */EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
		devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		HMODULE hUser32=LoadLibrary("USER32");
		if (hUser32) pd = (pEnumDisplayDevices)GetProcAddress( hUser32, "EnumDisplayDevicesA");
		if (pd)
			{
				LPSTR deviceName=NULL;
				DISPLAY_DEVICE dd;
				ZeroMemory(&dd, sizeof(dd));
				dd.cb = sizeof(dd);
				devmode.dmDeviceName[0] = '\0';
				INT devNum = 0;
				BOOL result;
				DriverFound=false;
				while ((result = (*pd)(NULL,devNum, &dd,0)))
				{
					if (strcmp((const char *)&dd.DeviceString[0], driverName) == 0)
					{
					DriverFound=true;
					break;
					}
					devNum++;
				}
				if (DriverFound)
				{
					if (hUser32) FreeLibrary(hUser32);
					if(Box)
					{
						char buf[512];
						GetDllProductVersion("mv2.dll",buf,512);
						if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
						{
							strcat(buf," driver Active");
							HDC testdc=NULL;
							deviceName = (LPSTR)&dd.DeviceName[0];
							testdc = CreateDC("DISPLAY",deviceName,NULL,NULL);	
							if (testdc)
							{
								DeleteDC(testdc);
								strcat(buf," access ok");
							}
							else
							{
								strcat(buf," access denied, permission problem");
							}
						}
						else
							strcat(buf," driver Not Active");
						    
						MessageBoxSecure(NULL,buf,"driver info: required version 1.22",0);
					}
					return true;
				}
				else if(Box) MessageBoxSecure(NULL,"Driver not found: Perhaps you need to reboot after install","driver info: required version 1.22",0);
			}
	if (hUser32) FreeLibrary(hUser32);	
	return false;
}
