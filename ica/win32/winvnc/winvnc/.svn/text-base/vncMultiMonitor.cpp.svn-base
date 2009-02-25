// System headers
#include <assert.h>
#include "stdhdrs.h"

// Custom headers
//#include <WinAble.h>
#include <omnithread.h>
#include "WinVNC.h"
#include "VNCHooks\VNCHooks.h"
#include "vncServer.h"
#include "vncKeymap.h"
#include "rfbRegion.h"
#include "rfbRect.h"
#include "vncDesktop.h"
#include "vncService.h"
// Modif rdv@2002 - v1.1.x - videodriver
#include "vncOSVersion.h"

#include "mmSystem.h" // sf@2002
#include "TextChat.h" // sf@2002
#include "vncdesktopthread.h"

void 
vncDesktop::Checkmonitors()
{
  nr_monitors=GetNrMonitors();
  DEVMODE devMode;
  if (nr_monitors>0)
  {
	if(OSversion()==1 || OSversion()==2 || OSversion()==4 )GetPrimaryDevice();
	devMode.dmSize = sizeof(DEVMODE);
	if(OSversion()==1 || OSversion()==2 || OSversion()==4 ) EnumDisplaySettings(mymonitor[0].device, ENUM_CURRENT_SETTINGS, &devMode);
	else EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
	mymonitor[0].offsetx=devMode.dmPosition.x;
	mymonitor[0].offsety=devMode.dmPosition.y;
	mymonitor[0].Width=devMode.dmPelsWidth;
	mymonitor[0].Height=devMode.dmPelsHeight;
	mymonitor[0].Depth=devMode.dmBitsPerPel;
  }
  if (nr_monitors>1)
  {
	GetSecondaryDevice();
	devMode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(mymonitor[1].device, ENUM_CURRENT_SETTINGS, &devMode);
	mymonitor[1].offsetx=devMode.dmPosition.x;
	mymonitor[1].offsety=devMode.dmPosition.y;
	mymonitor[1].Width=devMode.dmPelsWidth;
	mymonitor[1].Height=devMode.dmPelsHeight;
	mymonitor[1].Depth=devMode.dmBitsPerPel;
  }
	///
    mymonitor[2].offsetx=GetSystemMetrics(SM_XVIRTUALSCREEN);
    mymonitor[2].offsety=GetSystemMetrics(SM_YVIRTUALSCREEN);
    mymonitor[2].Width=GetSystemMetrics(SM_CXVIRTUALSCREEN);
    mymonitor[2].Height=GetSystemMetrics(SM_CYVIRTUALSCREEN);
	mymonitor[2].Depth=mymonitor[0].Depth;//depth primary monitor is used

}



int
vncDesktop::GetNrMonitors()
{
	if(OSversion()==3 || OSversion()==5) return 1;
	int i;
    int j=0;
	pEnumDisplayDevices pd;
    pd = (pEnumDisplayDevices)GetProcAddress( LoadLibrary("USER32"), "EnumDisplayDevicesA");

    if (pd)
    {
        DISPLAY_DEVICE dd;
        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);
        for (i=0; (pd)(NULL, i, &dd, 0); i++)
			{
				if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
					if (!(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))j++;
			}
	}
	return j;
}

void
vncDesktop::GetPrimaryDevice()
{
	int i;
    int j=0;
	pEnumDisplayDevices pd;
    pd = (pEnumDisplayDevices)GetProcAddress( LoadLibrary("USER32"), "EnumDisplayDevicesA");

    if (pd)
    {
        DISPLAY_DEVICE dd;
        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);
        for (i=0; (pd)(NULL, i, &dd, 0); i++)
			{
				if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
					if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
						if (!(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
						{
							strcpy(mymonitor[0].device,(char *)dd.DeviceName);
						}

			}
	}
}

void
vncDesktop::GetSecondaryDevice()
{
	int i;
    int j=0;
	pEnumDisplayDevices pd;
    pd = (pEnumDisplayDevices)GetProcAddress( LoadLibrary("USER32"), "EnumDisplayDevicesA");

    if (pd)
    {
        DISPLAY_DEVICE dd;
        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);
        for (i=0; (pd)(NULL, i, &dd, 0); i++)
			{
				if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
					if (!(dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE))
						if (!(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
						{
							strcpy(mymonitor[1].device,(char *)dd.DeviceName);
						}

			}
	}
}