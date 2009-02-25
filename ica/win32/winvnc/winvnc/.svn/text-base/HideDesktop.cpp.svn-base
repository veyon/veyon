//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
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
// Functions to hide the Windows Desktop
//
// This hides three variants:
//	- Desktop Patterns  (WIN.INI [Desktop] Pattern=)
//	- Desktop Wallpaper (.bmp [and JPEG on Windows XP])
//	- Active Desktop
//
// Written by Ed Hardebeck - Glance Networks, Inc.
// With some code from Paul DiLascia, MSDN Magazine - May 2001
//
//	HideDesktop()		- hides the desktop
//	RestoreDesktop()	- restore the desktop
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinInet.h> // Shell object uses INTERNET_MAX_URL_LENGTH (go figure)
#if _MSC_VER < 1400
#define _WIN32_IE 0x0400
#endif
#include <atlbase.h> // ATL smart pointers
#include <shlguid.h> // shell GUIDs
#include <shlobj.h>  // IActiveDesktop

struct __declspec(uuid("F490EB00-1240-11D1-9888-006097DEACF9")) IActiveDesktop;

#define PACKVERSION(major,minor) MAKELONG(minor,major)

DWORD GetDllVersion(LPCTSTR lpszDllName)
{
    HINSTANCE hinstDll;
    DWORD dwVersion = 0;

    hinstDll = LoadLibrary(lpszDllName);
	
    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;

        pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");

		// Because some DLLs might not implement this function,
		// test for it explicitly. Depending on the particular 
		// DLL, the lack of a DllGetVersion function can be an
		// indicator of the version.
		
        if (pDllGetVersion)
        {
            DLLVERSIONINFO	dvi;
            HRESULT			hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
                dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }
        
        FreeLibrary(hinstDll);
    }
    return dwVersion;
}

typedef void (CALLBACK * P_SHGetSettings)(LPSHELLFLAGSTATE lpsfs, DWORD dwMask);

BOOL SHDesktopHTML()
{
   // Determine if Active Desktop is enabled. SHGetSettings is apparently
   // more reliable than IActiveDesktop::GetDesktopItemOptions, which can
   // report incorrectly during the middle of ApplyChanges!
   // Dynamically link to SHGetVersion so this can load on Windows with pre 4.71 shell

	HINSTANCE	hinstDll;
    BOOL		enabled = FALSE;

    hinstDll = LoadLibrary("shell32.dll");
	
    if (hinstDll)
    {
	  P_SHGetSettings	pSHGetSettings;

	  if ((pSHGetSettings = (P_SHGetSettings)GetProcAddress(hinstDll, "SHGetSettings")))
	  {
		SHELLFLAGSTATE	shfs;

		(*pSHGetSettings)(&shfs, SSF_DESKTOPHTML);
		enabled = shfs.fDesktopHTML;
	  }

	  FreeLibrary(hinstDll);
	}

	return enabled;
}

static 
HRESULT EnableActiveDesktop(bool enable)
{
	CComQIPtr<IActiveDesktop, &IID_IActiveDesktop>	pIActiveDesktop;
	
	HRESULT		hr;

	CoInitialize(NULL);
	hr = pIActiveDesktop.CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER);
	if (!SUCCEEDED(hr))
		return hr;

	COMPONENTSOPT	opt;

	opt.dwSize = sizeof(opt);
	opt.fActiveDesktop = opt.fEnableComponents = enable;
    hr = pIActiveDesktop->SetDesktopItemOptions(&opt, 0);
    if (!SUCCEEDED(hr))
		return hr;

	return pIActiveDesktop->ApplyChanges(AD_APPLY_REFRESH);
}

bool HideActiveDesktop()
{
	// returns true if the Active Desktop was enabled and has been hidden

	if (GetDllVersion(TEXT("shell32.dll")) >= PACKVERSION(4,71))
		if (SHDesktopHTML())
			return SUCCEEDED(EnableActiveDesktop(false));

	return false;
}

void ShowActiveDesktop()
{
	if (GetDllVersion(TEXT("shell32.dll")) >= PACKVERSION(4,71))
		EnableActiveDesktop(true);
}
		
// OK, so this doesn't work in multiple threads or nest...
static TCHAR	DesktopPattern[40];
static BOOL		ADWasEnabled = false;
static BOOL		ISWallPaperHided = false;
static TCHAR SCREENNAME[1024];

// Added Jef Fix - 4 March 2008 jpaquette (paquette@atnetsend.net)
static unsigned char WallpaperStyle[10] = {0}; // string "0" centered, "1" tiled, "2" stretched

static const TCHAR * const DESKTOP_KEYNAME = _T("Control Panel\\Desktop");
static const TCHAR * const DESKTOP_WALLPAPER_STYLE_KEYNAME = _T("WallpaperStyle");

void SaveWallpaperStyle()
{
    HKEY hKey;

    if (::RegOpenKeyEx(HKEY_CURRENT_USER, DESKTOP_KEYNAME, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD sz = sizeof WallpaperStyle;
        ::RegQueryValueEx(hKey, DESKTOP_WALLPAPER_STYLE_KEYNAME, 0, 0, WallpaperStyle, &sz);
        ::RegCloseKey(hKey);
    }
}

void RestoreWallpaperStyle()
{
    HKEY hKey;

    if (::RegOpenKeyEx(HKEY_CURRENT_USER, DESKTOP_KEYNAME, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        DWORD sz = strlen((const char *)WallpaperStyle);
        ::RegSetValueEx(hKey, DESKTOP_WALLPAPER_STYLE_KEYNAME, 0, REG_SZ, WallpaperStyle, sz);
        ::RegCloseKey(hKey);
    }
}
// -- end jdp


void HideDesktop()
{
	//GetProfileString("Desktop", "Pattern", "0 0 0 0 0 0 0 0", DesktopPattern, sizeof(DesktopPattern));

	// @@@efh Setting the desktop pattern via pvParam works, but is undocumented (except by www.winehq.com)
	//SystemParametersInfo(SPI_SETDESKPATTERN, 0, "0 0 0 0 0 0 0 0", SPIF_SENDCHANGE);

	// Tell all applications that there is no wallpaper
	// Note that this doesn't change the wallpaper registry setting!
	// @@@efh On Win98 and Win95 this returns an error in the debug build (but not in release)...
	if (!ISWallPaperHided)
	{
		SaveWallpaperStyle(); // Added Jef Fix
		SystemParametersInfo(SPI_GETDESKWALLPAPER,1024,SCREENNAME,NULL );
		SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, "", SPIF_SENDCHANGE);
		ADWasEnabled = HideActiveDesktop();
		ISWallPaperHided=true;
	}
}

void RestoreDesktop()
{
	if (ISWallPaperHided)
	{
		if (ADWasEnabled)
			ShowActiveDesktop();

		SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, SCREENNAME, SPIF_SENDCHANGE);
		RestoreWallpaperStyle(); // Added Jef Fix
	}

	ISWallPaperHided=false;

	//SystemParametersInfo(SPI_SETDESKPATTERN, 0, DesktopPattern, SPIF_SENDCHANGE);
}