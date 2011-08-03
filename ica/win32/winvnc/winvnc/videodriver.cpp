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

#include "stdhdrs.h"
#include "videodriver.h"
#include <strsafe.h>
LPSTR driverName = "mv video hook driver2";

#define MAP1 1030
#define UNMAP1 1031
#define CURSOREN 1060
#define CURSORDIS 1061

VIDEODRIVER::VIDEODRIVER()
{
	mypVideoMemory=NULL;
	mypchangebuf=NULL;
	myframebuffer=NULL;
	shared_buffer_size=0;
}

void
VIDEODRIVER::VIDEODRIVER_start(int x,int y,int w,int h)
{
	blocked=true;
	oldaantal=1;
	mypVideoMemory=NULL;
	OSVER=OSVersion();
	if (OSVER==OSWIN2000|| OSVER==OSWINXP64)
	{
		if (Mirror_driver_attach_XP(x,y,w,h))
		{
			if (GetDcMirror()!=NULL)
			{
			mypVideoMemory=VideoMemory_GetSharedMemory();
			mypchangebuf=(PCHANGES_BUF)mypVideoMemory;
			myframebuffer=mypVideoMemory+sizeof(CHANGES_BUF);
			}
			else
			{
				mypVideoMemory=NULL;
			}
		}
		else
		{
			mypVideoMemory=NULL;
		}
	}
	if (OSVER==OSVISTA)
	{
		if (Mirror_driver_Vista(1,x,y,w,h))
		{
			if (GetDcMirror()!=NULL)
			{
			mypVideoMemory=VideoMemory_GetSharedMemory();
			mypchangebuf=(PCHANGES_BUF)mypVideoMemory;
			myframebuffer=mypVideoMemory+sizeof(CHANGES_BUF);
			//if (mypVideoMemory==NULL) MessageBoxSecure(NULL,"MVideo driver failed", NULL, MB_OK);
			}
			else
			{
	//			MessageBoxSecure(NULL,"Video driver failed", NULL, MB_OK);
////////////////////////////////////////////////////////////////////////////////
				if (Mirror_driver_attach_XP(x,y,w,h))
					{
						if (GetDcMirror()!=NULL)
						{
						mypVideoMemory=VideoMemory_GetSharedMemory();
						mypchangebuf=(PCHANGES_BUF)mypVideoMemory;
						myframebuffer=mypVideoMemory+sizeof(CHANGES_BUF);
						}
						else
						{
							mypVideoMemory=NULL;
						}
					}
					else
					{
						mypVideoMemory=NULL;
					}
////////////////////////////////////////////////////////////////////////////////
				//mypVideoMemory=NULL;
			}
		}
		else
		{
			mypVideoMemory=NULL;
		}
	}
	blocked=false;
}

void
VIDEODRIVER::VIDEODRIVER_Stop()
{
	OSVER=OSVersion();
	if (OSVER==OSWIN2000||OSVER==OSWINXP64)
	{
		Mirror_driver_detach_XP();
		if (mypVideoMemory!=NULL) VideoMemory_ReleaseSharedMemory(mypVideoMemory);
	}
	if (OSVER==OSVISTA)
	{
		Mirror_driver_Vista(0,0,0,0,0);
		if (mypVideoMemory!=NULL) VideoMemory_ReleaseSharedMemory(mypVideoMemory);
	}
	mypVideoMemory=NULL;
	mypchangebuf=NULL;
	myframebuffer=NULL;


}


VIDEODRIVER::~VIDEODRIVER()
{
	//mypVideoMemory=NULL;
	OSVER=OSVersion();
	if (OSVER==OSWIN2000|| OSVER==OSWINXP64)
	{
		Mirror_driver_detach_XP();
		if (mypVideoMemory!=NULL) VideoMemory_ReleaseSharedMemory(mypVideoMemory);
	}
	if (OSVER==OSVISTA)
	{
		Mirror_driver_Vista(0,0,0,0,0);
		if (mypVideoMemory!=NULL) VideoMemory_ReleaseSharedMemory(mypVideoMemory);
	}

}

void VIDEODRIVER::VideoMemory_ReleaseSharedMemory(PCHAR pVideoMemory)
{
    UnmapViewOfFile(pVideoMemory);
}

PCHAR VIDEODRIVER::VideoMemory_GetSharedMemory(void)
{
   PCHAR pVideoMemory=NULL;
   HANDLE hMapFile, hFile, hFile0,hFile1;
   hFile=NULL;
   
   hFile0 = CreateFile("c:\\video0.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
   hFile1 = CreateFile("c:\\video1.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

   if ((hFile0 && hFile0 != INVALID_HANDLE_VALUE) && !(hFile1 && hFile1 != INVALID_HANDLE_VALUE)) hFile=hFile0;
   if ((hFile1 && hFile1 != INVALID_HANDLE_VALUE) && !(hFile0 && hFile0 != INVALID_HANDLE_VALUE)) hFile=hFile1;
   if ((hFile0 && hFile0 != INVALID_HANDLE_VALUE) && (hFile1 && hFile1 != INVALID_HANDLE_VALUE))
   {
	   DWORD size0=GetFileSize(hFile0,NULL);
	   DWORD size1=GetFileSize(hFile1,NULL);
	   if (size0==shared_buffer_size) hFile=hFile0;
	   if (size1==shared_buffer_size) hFile=hFile1;
	    if (size1==shared_buffer_size && size0==shared_buffer_size)
		{
			//find last modification time
			FILETIME createt0, lastWriteTime0, lastAccessTime0;
			GetFileTime( hFile0, &createt0, &lastAccessTime0, &lastWriteTime0);
			FILETIME createt1, lastWriteTime1, lastAccessTime1;
			GetFileTime( hFile0, &createt1, &lastAccessTime1, &lastWriteTime1);
			LONG result=CompareFileTime(&lastWriteTime0,&lastWriteTime1);
			if (result==0 || result== 1) hFile=hFile0;
			else hFile=hFile1;

		}

   }


   if(hFile && hFile != INVALID_HANDLE_VALUE)
   {
       hMapFile = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);

       if(hMapFile && hMapFile != INVALID_HANDLE_VALUE)
       {
           pVideoMemory = (char *) MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
    
           CloseHandle(hMapFile);
       }

       CloseHandle(hFile);
   }
   else
   {
	   vnclog.Print(LL_INTERR, VNCLOG("Error video.dat \n")); 
   }
   
   return pVideoMemory;
}

bool
VIDEODRIVER::Mirror_driver_Vista(DWORD dwAttach,int x,int y,int w,int h)
{
	HDESK   hdeskInput=NULL;
    HDESK   hdeskCurrent=NULL;
	pEnumDisplayDevices pd=NULL;
	HMODULE hUser32=LoadLibrary("USER32");
	if (hUser32) pd = (pEnumDisplayDevices)GetProcAddress( hUser32, "EnumDisplayDevicesA");
	if (!pd) return false;
    BOOL  bED   = TRUE;
    DEVMODE devmode;

    FillMemory(&devmode, sizeof(DEVMODE), 0);
    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;

    // Make sure we have a display on this thread.
    BOOL change = EnumDisplaySettings(NULL,
                                      ENUM_CURRENT_SETTINGS,
                                      &devmode);

    devmode.dmFields = DM_BITSPERPEL |
                       DM_PELSWIDTH | 
                       DM_PELSHEIGHT |
                       DM_POSITION ;

    if (change) 
    {
        // query all display devices in the system until we hit a primary
        // display device. Using it get the width and height of the primary
        // so we can use that for the mirror driver. Also enumerate the
        // display devices installed on this machine untill we hit
        // our favourate mirrored driver, then extract the device name string
        // of the format '\\.\DISPLAY#'
//		MessageBoxSecure(NULL, "change ok", NULL, MB_OK);
   
        DISPLAY_DEVICE dispDevice;
   
        FillMemory(&dispDevice, sizeof(DISPLAY_DEVICE), 0);
   
        dispDevice.cb = sizeof(DISPLAY_DEVICE);
   
        LPSTR deviceName = NULL;

        devmode.dmDeviceName[0] = '\0';

        INT devNum = 0;
        BOOL result;
        DWORD cxPrimary = 0xFFFFFFFF;
        DWORD cyPrimary = 0xFFFFFFFF;

        // First enumerate for Primary display device:
        while ((result = (*pd)(NULL,
                                devNum,
                                &dispDevice,
                                0)))
        {
//          MessageBoxSecure(NULL, &dispDevice.DeviceString[0], NULL, MB_OK);
          if (dispDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
          {
              // Primary device. Find out its dmPelsWidht and dmPelsHeight.
              EnumDisplaySettings(dispDevice.DeviceName,
                                  ENUM_CURRENT_SETTINGS,
                                  &devmode);

              cxPrimary = devmode.dmPelsWidth;
              cyPrimary = devmode.dmPelsHeight;
              break;
          }
          devNum++;
        }
#if defined(MIRROR_24)
		if (devmode.dmBitsPerPel == 24)
			devmode.dmBitsPerPel = 32;
#endif
		if (devmode.dmBitsPerPel!=8 && devmode.dmBitsPerPel!=16 && devmode.dmBitsPerPel!=32) 
		{
			if (hUser32) FreeLibrary(hUser32);
			return false;
		}
		shared_buffer_size=devmode.dmBitsPerPel/8*w*h+sizeof(CHANGES_BUF);

        // error check
        if (!result)
        {
//           MessageBoxSecure(NULL,  driverName, NULL, MB_OK);
		   if (hUser32) FreeLibrary(hUser32);
           return false;
        }

        if (cxPrimary == 0xffffffff || cyPrimary == 0xffffffff)
        {
//             MessageBoxSecure(NULL,"cxPrimary or cyPrimary not valid", NULL, MB_OK);
			if (hUser32) FreeLibrary(hUser32);
            return false;
        }

        // Enumerate again for the mirror driver:
        devNum = 0;
        while ((result = (*pd)(NULL,
                                  devNum,
                                  &dispDevice,
                                  0)))
        {
          if (strcmp(&dispDevice.DeviceString[0], driverName) == 0)
              break;

           devNum++;
        }

        // error check       
        if (!result)
        {
//          MessageBoxSecure(NULL,driverName, NULL, MB_OK);
		   if (hUser32) FreeLibrary(hUser32);
           return false;
        }

//        printf("DevNum:%d\nName:%s\nString:%s\nID:%s\nKey:%s\n\n",
//               devNum,
//               &dispDevice.DeviceName[0],
//               &dispDevice.DeviceString[0],
//               &dispDevice.DeviceID[0],
//               &dispDevice.DeviceKey[0]);

        CHAR deviceNum[MAX_PATH];
        LPSTR deviceSub;

        // Simply extract 'DEVICE#' from registry key.  This will depend
        // on how many mirrored devices your driver has and which ones
        // you intend to use.

        _strupr(&dispDevice.DeviceKey[0]);

        deviceSub = strstr(&dispDevice.DeviceKey[0],
                           "\\DEVICE");

        if (!deviceSub) 
            StringCbCopy(&deviceNum[0], sizeof(deviceNum), "DEVICE0");
        else
            StringCbCopy(&deviceNum[0], sizeof(deviceNum), ++deviceSub);

        // Reset the devmode for mirror driver use:
		int depth=devmode.dmBitsPerPel;
        FillMemory(&devmode, sizeof(DEVMODE), 0);
        devmode.dmSize = sizeof(DEVMODE);
        devmode.dmDriverExtra = 0;
		devmode.dmBitsPerPel=depth;

        devmode.dmFields = DM_BITSPERPEL |
                           DM_PELSWIDTH | 
                           DM_PELSHEIGHT |
                           DM_POSITION;

        StringCbCopy((LPSTR)&devmode.dmDeviceName[0], sizeof(devmode.dmDeviceName), "mv2");
        deviceName = (LPSTR)&dispDevice.DeviceName[0];

        if (bED)
        {
            // Attach and detach information is sent via the dmPelsWidth/Height
            // of the devmode.
            //
//			MessageBoxSecure(NULL,"bED", NULL, MB_OK);
            if (dwAttach == 0)
            {
                devmode.dmPelsWidth = 0;
                devmode.dmPelsHeight = 0;
            }
            else
            {
                devmode.dmPelsWidth=w;
				devmode.dmPelsHeight=h;
				devmode.dmPosition.x=x;
				devmode.dmPosition.y=y;
            }
            // Update the mirror device's registry data with the devmode. Dont
            // do a mode change.
			
/////////////////////////
				hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
				if (hdeskCurrent != NULL)
					{
						hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
						if (hdeskInput != NULL)
							{
								SetThreadDesktop(hdeskInput);
							}
					}


				INT code =
            ChangeDisplaySettingsEx(deviceName,
                                    &devmode, 
                                    NULL,
                                    (CDS_UPDATEREGISTRY | CDS_RESET|CDS_GLOBAL),
                                    NULL
                                    );
//            GetDispCode(code);
			if (code!=0) 
			{
				if (hUser32) FreeLibrary(hUser32);
				return false;
			}
            // Now do the real mode change to take mirror driver changes into
            // effect.
            code = ChangeDisplaySettingsEx(NULL,
                                           NULL,
                                           NULL,
                                           0,
                                           NULL);

			if (hdeskCurrent) SetThreadDesktop(hdeskCurrent);
			if (hdeskInput) CloseDesktop(hdeskInput);

			if (code!=0)
			{
				if (hUser32) FreeLibrary(hUser32);
				return false;
			}
			if (hUser32) FreeLibrary(hUser32);
			return true;
        }
		if (hUser32) FreeLibrary(hUser32);
		return false;
	}
	if (hUser32) FreeLibrary(hUser32);
	return false;
}



void
VIDEODRIVER::Mirror_driver_detach_XP()
{
	HDESK   hdeskInput=NULL;
    HDESK   hdeskCurrent=NULL;
	pEnumDisplayDevices pd=NULL;
	HMODULE hUser32=LoadLibrary("USER32");
	if (hUser32) pd = (pEnumDisplayDevices)GetProcAddress( hUser32, "EnumDisplayDevicesA");
	if (!pd) return;
	DEVMODE devmode;

    FillMemory(&devmode, sizeof(DEVMODE), 0);

    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;

    BOOL change = EnumDisplaySettings(NULL,
                                      ENUM_CURRENT_SETTINGS,
                                      &devmode);

    devmode.dmFields = DM_BITSPERPEL |
                       DM_PELSWIDTH | 
                       DM_PELSHEIGHT;

    if (change) 
    {
        // query all display devices in the system until we hit
        // our favourate mirrored driver, then extract the device name string
        // of the format '\\.\DISPLAY#'
       
        DISPLAY_DEVICE dispDevice;
       
        FillMemory(&dispDevice, sizeof(DISPLAY_DEVICE), 0);
       
        dispDevice.cb = sizeof(DISPLAY_DEVICE);
       
        LPSTR deviceName = NULL;

        devmode.dmDeviceName[0] = '\0';

        INT devNum = 0;
        BOOL result;

        while ((result = (*pd)(NULL,
                                  devNum,
                                  &dispDevice,
                                  0)))
        {
          if (strcmp(&dispDevice.DeviceString[0], driverName) == 0)
              break;

           devNum++;
        }
       
        if (!result)
        {
           printf("No '%s' found.\n", driverName);
		   if (hUser32) FreeLibrary(hUser32);
           return;
        }

        printf("DevNum:%d\nName:%s\nString:%s\nID:%s\nKey:%s\n\n",
               devNum,
               &dispDevice.DeviceName[0],
               &dispDevice.DeviceString[0],
               &dispDevice.DeviceID[0],
               &dispDevice.DeviceKey[0]);

        CHAR deviceNum[MAX_PATH];
        LPSTR deviceSub;

        // Simply extract 'DEVICE#' from registry key.  This will depend
        // on how many mirrored devices your driver has and which ones
        // you intend to use.

        _strupr(&dispDevice.DeviceKey[0]);

        deviceSub = strstr(&dispDevice.DeviceKey[0],
                           "\\DEVICE");

        if (!deviceSub) 
            StringCbCopy(&deviceNum[0], sizeof(deviceNum), "DEVICE0");
        else
            StringCbCopy(&deviceNum[0], sizeof(deviceNum), ++deviceSub);
        
        // Add 'Attach.ToDesktop' setting.
        //
		{
				HKEY hKeyProfileMirror = (HKEY)0;
				if (RegCreateKey(HKEY_LOCAL_MACHINE,
								_T("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\mv2"),
								 &hKeyProfileMirror) != ERROR_SUCCESS)
				{
				   if (hUser32) FreeLibrary(hUser32);
				   return;
				}

				HKEY hKeyDevice = (HKEY)0;
				if (RegCreateKey(hKeyProfileMirror,
								 _T(&deviceNum[0]),
								 &hKeyDevice) != ERROR_SUCCESS)
				{
				   if (hUser32) FreeLibrary(hUser32);
				   return;
				}

				DWORD one = 0;
				if (RegSetValueEx(hKeyDevice,
								  _T("Attach.ToDesktop"),
								  0,
								  REG_DWORD,
								  (unsigned char *)&one,
								  4) != ERROR_SUCCESS)
				{
				   if (hUser32) FreeLibrary(hUser32);
				   return;
				}
				RegCloseKey(hKeyProfileMirror);
				RegCloseKey(hKeyDevice);
		}
		{
				deviceSub = strstr(&dispDevice.DeviceKey[0],
                           "SYSTEM");

				HKEY hKeyProfileMirror = (HKEY)0;
				if (RegCreateKey(HKEY_LOCAL_MACHINE,
								_T("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current"),
								 &hKeyProfileMirror) != ERROR_SUCCESS)
				{
				   if (hUser32) FreeLibrary(hUser32);
				   return;
				}

				HKEY hKeyDevice = (HKEY)0;
				if (RegCreateKey(hKeyProfileMirror,
								 _T(deviceSub),
								 &hKeyDevice) != ERROR_SUCCESS)
				{
				   if (hUser32) FreeLibrary(hUser32);
				   return;
				}

				DWORD one = 0;
				if (RegSetValueEx(hKeyDevice,
								  _T("Attach.ToDesktop"),
								  0,
								  REG_DWORD,
								  (unsigned char *)&one,
								  4) != ERROR_SUCCESS)
				{
				   if (hUser32) FreeLibrary(hUser32);
				   return;
				}
				RegCloseKey(hKeyProfileMirror);
				RegCloseKey(hKeyDevice);
		}


        FillMemory(&devmode, sizeof(DEVMODE), 0);

        devmode.dmSize = sizeof(DEVMODE);
        devmode.dmDriverExtra = 0;

        StringCbCopy((LPSTR)&devmode.dmDeviceName[0], sizeof(devmode.dmDeviceName), "mv2");
        deviceName = (LPSTR)&dispDevice.DeviceName[0];
		devmode.dmBitsPerPel=32;
        // add 'Default.*' settings to the registry under above hKeyProfile\mirror\device
		hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
				if (hdeskCurrent != NULL)
					{
						hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
						if (hdeskInput != NULL)
							{
								SetThreadDesktop(hdeskInput);
							}
					}

        INT code =
        ChangeDisplaySettingsEx(deviceName,
                                &devmode, 
                                NULL,
                                CDS_UPDATEREGISTRY,
                                NULL
                                );
    
//        GetDispCode(code);

        code = ChangeDisplaySettingsEx(deviceName,
                                &devmode, 
                                NULL,
                                0,
                                NULL
                                );
   
		if (hdeskCurrent) SetThreadDesktop(hdeskCurrent);
		if (hdeskInput) CloseDesktop(hdeskInput);
		
	}
if (hUser32) FreeLibrary(hUser32);
}

//BOOL
//Activate_video_driver();

bool
VIDEODRIVER::Mirror_driver_attach_XP(int x,int y,int w,int h)
{
	HDESK   hdeskInput=NULL;
    HDESK   hdeskCurrent=NULL;

	pEnumDisplayDevices pd=NULL;
	HMODULE hUser32=LoadLibrary("USER32");
	if (hUser32) pd = (pEnumDisplayDevices)GetProcAddress( hUser32, "EnumDisplayDevicesA");
	if (!pd) return false;

	DEVMODE devmode;

    FillMemory(&devmode, sizeof(DEVMODE), 0);

    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;

    BOOL change = EnumDisplaySettings(NULL,
                                      ENUM_CURRENT_SETTINGS,
                                      &devmode);

    devmode.dmFields = DM_BITSPERPEL |
                       DM_PELSWIDTH | 
					   DM_POSITION |
                       DM_PELSHEIGHT;

#if defined(MIRROR_24)
	// for 24 bit mode, have the mirror driver use 32 bit.
	if (devmode.dmBitsPerPel == 24)
		devmode.dmBitsPerPel = 32;
#endif
	if (devmode.dmBitsPerPel!=8 && devmode.dmBitsPerPel!=16 && devmode.dmBitsPerPel!=32)
	{
		if (hUser32) FreeLibrary(hUser32);
		return false;
	}

	shared_buffer_size=devmode.dmBitsPerPel/8*w*h+sizeof(CHANGES_BUF);

    if (change) 
    {
        // query all display devices in the system until we hit
        // our favourate mirrored driver, then extract the device name string
        // of the format '\\.\DISPLAY#'
       
        DISPLAY_DEVICE dispDevice;
       
        FillMemory(&dispDevice, sizeof(DISPLAY_DEVICE), 0);
       
        dispDevice.cb = sizeof(DISPLAY_DEVICE);
       
        LPSTR deviceName = NULL;

        devmode.dmDeviceName[0] = '\0';

        INT devNum = 0;
        BOOL result;

        while ((result = (*pd)(NULL,
                                  devNum,
                                  &dispDevice,
                                  0)))
        {
          if (strcmp(&dispDevice.DeviceString[0], driverName) == 0)
              break;

           devNum++;
        }
       
        if (!result)
        {
           printf("No '%s' found.\n", driverName);
		   if (hUser32) FreeLibrary(hUser32);
           return false;
        }

        printf("DevNum:%d\nName:%s\nString:%s\nID:%s\nKey:%s\n\n",
               devNum,
               &dispDevice.DeviceName[0],
               &dispDevice.DeviceString[0],
               &dispDevice.DeviceID[0],
               &dispDevice.DeviceKey[0]);

        CHAR deviceNum[MAX_PATH];
        LPSTR deviceSub;

        // Simply extract 'DEVICE#' from registry key.  This will depend
        // on how many mirrored devices your driver has and which ones
        // you intend to use.

        _strupr(&dispDevice.DeviceKey[0]);

        deviceSub = strstr(&dispDevice.DeviceKey[0],
                           "\\DEVICE");

        if (!deviceSub) 
            StringCbCopy(&deviceNum[0], MAX_PATH, "DEVICE0");
        else
            StringCbCopy(&deviceNum[0], MAX_PATH, ++deviceSub);
        
        // Add 'Attach.ToDesktop' setting.
        //

        HKEY hKeyProfileMirror = (HKEY)0;
        if (RegCreateKey(HKEY_LOCAL_MACHINE,
                        _T("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\mv2"),
                         &hKeyProfileMirror) != ERROR_SUCCESS)
        {
		   if (hUser32) FreeLibrary(hUser32);
           return false;
        }

        HKEY hKeyDevice = (HKEY)0;
        if (RegCreateKey(hKeyProfileMirror,
                         _T(&deviceNum[0]),
                         &hKeyDevice) != ERROR_SUCCESS)
        {
		   if (hUser32) FreeLibrary(hUser32);
           return false;
        }

        DWORD one = 1;
        if (RegSetValueEx(hKeyDevice,
                          _T("Attach.ToDesktop"),
                          0,
                          REG_DWORD,
                          (unsigned char *)&one,
                          4) != ERROR_SUCCESS)
        {
		   if (hUser32) FreeLibrary(hUser32);
           return false;
        }
		int depth=devmode.dmBitsPerPel;

        FillMemory(&devmode, sizeof(DEVMODE), 0);

        devmode.dmSize = sizeof(DEVMODE);
        devmode.dmDriverExtra = 0;

        StringCbCopy((LPSTR)&devmode.dmDeviceName[0], 32, "mv2");

        devmode.dmFields = DM_BITSPERPEL |
                           DM_PELSWIDTH | 
		    			   DM_POSITION |
                           DM_PELSHEIGHT;
        deviceName = (LPSTR)&dispDevice.DeviceName[0];
		devmode.dmPelsWidth=w;
		devmode.dmPelsHeight=h;
		devmode.dmPosition.x=x;
		devmode.dmPosition.y=y;
		devmode.dmBitsPerPel=depth;

		hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
				if (hdeskCurrent != NULL)
					{
						hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
						if (hdeskInput != NULL)
							{
								SetThreadDesktop(hdeskInput);
							}
					}

        // add 'Default.*' settings to the registry under above hKeyProfile\mirror\device
        INT code =
        ChangeDisplaySettingsEx(deviceName,
                                &devmode, 
                                NULL,
                                CDS_UPDATEREGISTRY,
                                NULL
                                );
    
//        GetDispCode(code);
		if (code!=0) 
		{
			if (hUser32) FreeLibrary(hUser32);
			return false;
		}
        code = ChangeDisplaySettingsEx(deviceName,
                                &devmode, 
                                NULL,
                                0,
                                NULL
                                );
   
		if (hdeskCurrent)SetThreadDesktop(hdeskCurrent);
		if (hdeskInput)CloseDesktop(hdeskInput);

		RegCloseKey(hKeyProfileMirror);
        RegCloseKey(hKeyDevice);
		if (code!=0)
		{
			if (hUser32) FreeLibrary(hUser32);
			return false;
		}
		if (hUser32) FreeLibrary(hUser32);
		return true;
	}
	if (hUser32) FreeLibrary(hUser32);
	return false;
}

int
VIDEODRIVER::OSVersion()
{
   OSVERSIONINFOEX osvi;
   SYSTEM_INFO si;
   PGNSI pGNSI;
   BOOL bOsVersionInfoEx;

   ZeroMemory(&si, sizeof(SYSTEM_INFO));
   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
   {
      osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
      if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) 
         return FALSE;
   }
   pGNSI = (PGNSI) GetProcAddress(
      GetModuleHandle(TEXT("kernel32.dll")), 
      "GetNativeSystemInfo");
   if(NULL != pGNSI)
      pGNSI(&si);
   else GetSystemInfo(&si);

	switch (osvi.dwPlatformId)
   {
      case VER_PLATFORM_WIN32_NT:

      // Test for the specific product.

      if ( osvi.dwMajorVersion == 6)
      {
         return OSVISTA;
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
      {
         if( osvi.wProductType == VER_NT_WORKSTATION &&
                   si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
         {
            return OSWINXP64;
         }
         else return OSWIN2003;
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
         return OSWINXP;

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
         return OSWIN2000;

      if ( osvi.dwMajorVersion <= 4 )
         return OSOLD;

	}
	return OSOLD;
}


HDC
VIDEODRIVER::GetDcMirror()
{
typedef BOOL (WINAPI* pEnumDisplayDevices)(PVOID,DWORD,PVOID,DWORD);
		HDC m_hrootdc=NULL;
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
				deviceName = (LPSTR)&dd.DeviceName[0];
				m_hrootdc = CreateDC("DISPLAY",deviceName,NULL,NULL);	
				if (m_hrootdc) DeleteDC(m_hrootdc);
				}
			}
		if (hUser32) FreeLibrary(hUser32);

		return m_hrootdc;
}

BOOL
VIDEODRIVER:: HardwareCursor()
{
	HDC gdc;
	int returnvalue;
	gdc = GetDC(NULL);
	returnvalue= ExtEscape(gdc, MAP1, 0, NULL, 0, NULL);
	returnvalue= ExtEscape(gdc, CURSOREN, 0, NULL, 0, NULL);
	ReleaseDC(NULL,gdc);
	return true;
}

BOOL
VIDEODRIVER:: NoHardwareCursor()
{
	HDC gdc;
	int returnvalue;
	gdc = GetDC(NULL);
	returnvalue= ExtEscape(gdc, CURSORDIS, 0, NULL, 0, NULL);
	ReleaseDC(NULL,gdc);
	return true;
}

