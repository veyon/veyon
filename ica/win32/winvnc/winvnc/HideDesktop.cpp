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
#include <shlwapi.h>
#include <windows.h>
#include <wininet.h> // Shell object uses INTERNET_MAX_URL_LENGTH (go figure)
#if !__GNUC__ && _MSC_VER < 1400
#define _WIN32_IE 0x0400
#endif
#include <tchar.h>
#include <shlguid.h> // shell GUIDs
#include <shlobj.h>  // IActiveDesktop
#include "stdhdrs.h"

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
#ifndef ULTRAVNC_ITALC_SUPPORT
	CoInitialize(NULL);
	CComQIPtr<IActiveDesktop, &IID_IActiveDesktop>	pIActiveDesktop;
	
	HRESULT		hr;

	hr = pIActiveDesktop.CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER);
	if (!SUCCEEDED(hr))
		return hr;

	COMPONENTSOPT	opt;

	opt.dwSize = sizeof(opt);
	opt.fActiveDesktop = opt.fEnableComponents = enable;
    hr = pIActiveDesktop->SetDesktopItemOptions(&opt, 0);
    if (!SUCCEEDED(hr))
	{
		CoUninitialize();
		return hr;
	}

	hr = pIActiveDesktop->ApplyChanges(AD_APPLY_REFRESH);
	CoUninitialize();
	return hr;
#else
	return 0;
#endif
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
	// @@@efh Setting the desktop pattern via pvParam works, but is undocumented (except by www.winehq.com)
	//SystemParametersInfo(SPI_SETDESKPATTERN, 0, "0 0 0 0 0 0 0 0", SPIF_SENDCHANGE);

	// Tell all applications that there is no wallpaper
	// Note that this doesn't change the wallpaper registry setting!
	// @@@efh On Win98 and Win95 this returns an error in the debug build (but not in release)...
	vnclog.Print(LL_INTWARN, VNCLOG("Killwallpaper %i\n"),ISWallPaperHided);
	if (!ISWallPaperHided)
	{
		SaveWallpaperStyle(); // Added Jef Fix
		SystemParametersInfo(SPI_GETDESKWALLPAPER,1024,SCREENNAME,0 );
		SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)"", SPIF_SENDCHANGE);
		ADWasEnabled = HideActiveDesktop();
		ISWallPaperHided=true;
		vnclog.Print(LL_INTWARN, VNCLOG("Killwallpaper %i %i\n"),ISWallPaperHided,ADWasEnabled);
	}
	
}

void RestoreDesktop()
{
	vnclog.Print(LL_INTWARN, VNCLOG("Restorewallpaper %i\n"),ISWallPaperHided);
	if (ISWallPaperHided)
	{
		vnclog.Print(LL_INTWARN, VNCLOG("Restorewallpaper %i %i\n"),ISWallPaperHided,ADWasEnabled);
		if (ADWasEnabled)
			ShowActiveDesktop();

		SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, SCREENNAME, SPIF_SENDCHANGE);
		RestoreWallpaperStyle(); // Added Jef Fix
	}

	ISWallPaperHided=false;
}


// adzm - 2010-07 - Disable more effects or font smoothing
// SPI_GETUIEFFECTS can enable/disable everything in one shot, but I'm sure we'll want finer control over this eventually.

UINT spiParams[] = {
	0x1024, //SPI_GETDROPSHADOW, 
	SPI_GETCOMBOBOXANIMATION, 
	SPI_GETMENUANIMATION, 
	SPI_GETTOOLTIPANIMATION, 
	SPI_GETSELECTIONFADE, 
	SPI_GETLISTBOXSMOOTHSCROLLING, 
	SPI_GETHOTTRACKING,
	SPI_GETGRADIENTCAPTIONS,
	0x1042, //SPI_GETCLIENTAREAANIMATION
	0x1043, //SPI_GETDISABLEOVERLAPPEDCONTENT
};


UINT spiSuggested[] = {
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
};

BOOL spiValues[(sizeof(spiParams) / sizeof(spiParams[0]))];

static bool g_bEffectsDisabled = false;

// adzm - 2010-07 - Disable more effects or font smoothing
void DisableEffects()
{
	if (!g_bEffectsDisabled) {
		g_bEffectsDisabled = true;

		::ZeroMemory(spiValues, sizeof(spiValues));

		for (size_t iParam = 0; iParam < (sizeof(spiParams) / sizeof(spiParams[0])); iParam++) {
			if (!SystemParametersInfo(spiParams[iParam], 0, &(spiValues[iParam]), 0)) {
				vnclog.Print(LL_INTWARN, VNCLOG("Failed to get SPI value for 0x%04x (0x%08x)\n"), spiParams[iParam], GetLastError());
			} else {
				vnclog.Print(LL_INTINFO, VNCLOG("Retrieved SPI value for 0x%04x: 0x%08x\n"), spiParams[iParam], spiValues[iParam]);
			}
		}
		for (size_t iParam = 0; iParam < (sizeof(spiParams) / sizeof(spiParams[0])); iParam++) {
			if (spiValues[iParam] != (BOOL) spiSuggested[iParam]) {
				if (!SystemParametersInfo(spiParams[iParam]+1, 0, (PVOID)spiSuggested[iParam], SPIF_SENDCHANGE)) {				
					vnclog.Print(LL_INTWARN, VNCLOG("Failed to set SPI value for 0x%04x to 0x%08x (0x%08x)\n"), spiParams[iParam]+1, spiSuggested[iParam], GetLastError());
				} else {
					vnclog.Print(LL_INTINFO, VNCLOG("Set SPI value for 0x%04x to 0x%08x\n"), spiParams[iParam]+1, spiSuggested[iParam]);
				}
			}
		}
	}
}

// adzm - 2010-07 - Disable more effects or font smoothing
void EnableEffects()
{
	if (g_bEffectsDisabled) {
		for (size_t iParam = 0; iParam < (sizeof(spiParams) / sizeof(spiParams[0])); iParam++) {
			if (spiValues[iParam] != (BOOL) spiSuggested[iParam]) {
				if (!SystemParametersInfo(spiParams[iParam]+1, 0, (PVOID)spiValues[iParam], SPIF_SENDCHANGE)) {
					vnclog.Print(LL_INTWARN, VNCLOG("Failed to restore SPI value for 0x%04x (0x%08x)\n"), spiParams[iParam]+1, GetLastError());
				} else {
					vnclog.Print(LL_INTINFO, VNCLOG("Restored SPI value for 0x%04x to 0x%08x\n"), spiParams[iParam]+1, spiValues[iParam]);
				}
			}
		}

		::ZeroMemory(spiValues, sizeof(spiValues));
	}

	g_bEffectsDisabled = false;
}



// adzm - 2010-07 - Disable more effects or font smoothing
//
bool g_bDisabledFontSmoothing = false;

bool g_bGotOldFontSmoothingValue = false;
bool g_bGotFontSmoothingType = false;
bool g_bGotClearType = false;
BOOL g_bOldClearTypeValue = TRUE;
BOOL g_bOldFontSmoothingValue = TRUE;
UINT g_nOldFontSmoothingType = 0x0001; // FE_FONTSMOOTHINGSTANDARD

// adzm - 2010-07 - Disable more effects or font smoothing
void DisableFontSmoothing()
{
	if (!g_bDisabledFontSmoothing) {
		
		if (!g_bGotOldFontSmoothingValue) {
			// there appears to be a delay between setting the SPI_SETFONTSMOOTHING value and SPI_GETFONTSMOOTHING returning that up-to-date value.
			if (!SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &g_bOldFontSmoothingValue, 0)) {
				vnclog.Print(LL_INTWARN, VNCLOG("Failed to get SPI value for SPI_GETFONTSMOOTHING (0x%08x)\n"), GetLastError());
				g_bGotOldFontSmoothingValue = false;
			} else {
				vnclog.Print(LL_INTINFO, VNCLOG("Retrieved SPI value for SPI_GETFONTSMOOTHING: 0x%08x\n"), g_bOldFontSmoothingValue);
				g_bGotOldFontSmoothingValue = true;
				
				if (!SystemParametersInfo(0x200A /*SPI_GETFONTSMOOTHINGTYPE*/, 0, &g_nOldFontSmoothingType, 0)) {
					vnclog.Print(LL_INTWARN, VNCLOG("Failed to get SPI value for SPI_GETFONTSMOOTHINGTYPE (0x%08x)\n"), GetLastError());
					g_bGotFontSmoothingType = false;
				} else {
					vnclog.Print(LL_INTINFO, VNCLOG("Retrieved SPI value for SPI_GETFONTSMOOTHINGTYPE: 0x%08x\n"), g_nOldFontSmoothingType);
					g_bGotFontSmoothingType = true;
				}
				
				if (!SystemParametersInfo(0x1048 /*SPI_GETCLEARTYPE*/, 0, &g_bOldClearTypeValue, 0)) {
					vnclog.Print(LL_INTWARN, VNCLOG("Failed to get SPI value for SPI_GETCLEARTYPE (0x%08x)\n"), GetLastError());
					g_bGotClearType = false;
				} else {
					vnclog.Print(LL_INTINFO, VNCLOG("Retrieved SPI value for SPI_GETCLEARTYPE: 0x%08x\n"), g_bOldClearTypeValue);
					g_bGotClearType = true;
				}
			}
		}


		if (g_bGotOldFontSmoothingValue && g_bOldFontSmoothingValue != FALSE) {
			
			if (g_bGotClearType && g_bOldClearTypeValue != FALSE) {
				if (!SystemParametersInfo(0x1049 /*SPI_SETCLEARTYPE*/, 0, (PVOID)FALSE, SPIF_SENDCHANGE)) {				
					vnclog.Print(LL_INTWARN, VNCLOG("Failed to set SPI value for SPI_SETCLEARTYPE (0x%08x)\n"), GetLastError());
				} else {
					g_bDisabledFontSmoothing = true; // ensure we reset even if SPI_SETFONTSMOOTHING fails for some reason
					vnclog.Print(LL_INTINFO, VNCLOG("Set SPI value for SPI_SETCLEARTYPE: 0x%08x\n"), FALSE);
				}
			}

			if (!SystemParametersInfo(SPI_SETFONTSMOOTHING, FALSE, NULL, SPIF_SENDCHANGE)) {				
				vnclog.Print(LL_INTWARN, VNCLOG("Failed to set SPI value for SPI_SETFONTSMOOTHING (0x%08x)\n"), GetLastError());
			} else {
				g_bDisabledFontSmoothing = true;
				vnclog.Print(LL_INTINFO, VNCLOG("Set SPI value for SPI_SETFONTSMOOTHING: 0x%08x\n"), FALSE);
			}
		}
	}
}

// adzm - 2010-07 - Disable more effects or font smoothing
void EnableFontSmoothing()
{
	if (g_bDisabledFontSmoothing && g_bGotOldFontSmoothingValue) {
		if (g_bOldFontSmoothingValue != FALSE) {
			if (!SystemParametersInfo(SPI_SETFONTSMOOTHING, g_bOldFontSmoothingValue, NULL, SPIF_SENDCHANGE)) {				
				vnclog.Print(LL_INTWARN, VNCLOG("Failed to restore SPI value for SPI_SETFONTSMOOTHING (0x%08x)\n"), GetLastError());
			} else {
				vnclog.Print(LL_INTINFO, VNCLOG("Restored SPI value for SPI_SETFONTSMOOTHING: 0x%08x\n"), g_bOldFontSmoothingValue);

				if (g_bGotClearType) {
					if (!SystemParametersInfo(0x1049 /*SPI_SETCLEARTYPE*/, 0, (PVOID)g_bOldClearTypeValue, SPIF_SENDCHANGE)) {				
						vnclog.Print(LL_INTWARN, VNCLOG("Failed to restore SPI value for SPI_SETCLEARTYPE (0x%08x)\n"), GetLastError());
					} else {
						vnclog.Print(LL_INTINFO, VNCLOG("Restored SPI value for SPI_SETCLEARTYPE: 0x%08x\n"), g_bOldClearTypeValue);
					}
				}

				if (g_bGotFontSmoothingType) {
					if (!SystemParametersInfo(0x200B /*SPI_SETFONTSMOOTHINGTYPE*/, 0, (PVOID)g_nOldFontSmoothingType, SPIF_SENDCHANGE)) {				
						vnclog.Print(LL_INTWARN, VNCLOG("Failed to restore SPI value for SPI_SETFONTSMOOTHINGTYPE (0x%08x)\n"), GetLastError());
					} else {
						vnclog.Print(LL_INTINFO, VNCLOG("Restored SPI value for SPI_SETFONTSMOOTHINGTYPE: 0x%08x\n"), g_nOldFontSmoothingType);
					}
				}
			}
		}
	}

	g_bDisabledFontSmoothing = false;
}
