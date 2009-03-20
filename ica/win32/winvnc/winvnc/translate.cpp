/*
 * translate.c - translate between different pixel formats
 */

/*
 *  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include "stdhdrs.h"

#include "translate.h"
#include <stdio.h>
#include "rfb.h"
#include "vncOSVersion.h"

#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#define CONCAT4(a,b,c,d) a##b##c##d
#define CONCAT4E(a,b,c,d) CONCAT4(a,b,c,d)

#define OUTVNC 8
#include "tableinittctemplate.cpp"
#include "tableinitcmtemplate.cpp"
#define INVNC 8
#include "tabletranstemplate.cpp"
#undef INVNC
#define INVNC 16
#include "tabletranstemplate.cpp"
#undef INVNC
#define INVNC 32
#include "tabletranstemplate.cpp"
#undef INVNC
#undef OUTVNC

#define OUTVNC 16
#include "tableinittctemplate.cpp"
#include "tableinitcmtemplate.cpp"
#define INVNC 8
#include "tabletranstemplate.cpp"
#undef INVNC
#define INVNC 16
#include "tabletranstemplate.cpp"
#undef INVNC
#define INVNC 32
#include "tabletranstemplate.cpp"
#undef INVNC
#undef OUTVNC

#define OUTVNC 32
#include "tableinittctemplate.cpp"
#include "tableinitcmtemplate.cpp"
#define INVNC 8
#include "tabletranstemplate.cpp"
#undef INVNC
#define INVNC 16
#include "tabletranstemplate.cpp"
#undef INVNC
#define INVNC 32
#include "tabletranstemplate.cpp"
#undef INVNC
#undef OUTVNC

rfbInitTableFnType rfbInitTrueColourSingleTableFns[3] = {
    rfbInitTrueColourSingleTable8,
    rfbInitTrueColourSingleTable16,
    rfbInitTrueColourSingleTable32
};

rfbInitTableFnType rfbInitColourMapSingleTableFns[3] = {
    rfbInitColourMapSingleTable8,
    rfbInitColourMapSingleTable16,
    rfbInitColourMapSingleTable32
};

rfbInitTableFnType rfbInitTrueColourRGBTablesFns[3] = {
    rfbInitTrueColourRGBTables8,
    rfbInitTrueColourRGBTables16,
    rfbInitTrueColourRGBTables32
};

rfbTranslateFnType rfbTranslateWithSingleTableFns[3][3] = {
    { rfbTranslateWithSingleTable8to8,
      rfbTranslateWithSingleTable8to16,
      rfbTranslateWithSingleTable8to32 },
    { rfbTranslateWithSingleTable16to8,
      rfbTranslateWithSingleTable16to16,
      rfbTranslateWithSingleTable16to32 },
    { rfbTranslateWithSingleTable32to8,
      rfbTranslateWithSingleTable32to16,
      rfbTranslateWithSingleTable32to32 }
};

rfbTranslateFnType rfbTranslateWithRGBTablesFns[3][3] = {
    { rfbTranslateWithRGBTables8to8,
      rfbTranslateWithRGBTables8to16,
      rfbTranslateWithRGBTables8to32 },
    { rfbTranslateWithRGBTables16to8,
      rfbTranslateWithRGBTables16to16,
      rfbTranslateWithRGBTables16to32 },
    { rfbTranslateWithRGBTables32to8,
      rfbTranslateWithRGBTables32to16,
      rfbTranslateWithRGBTables32to32 }
};



// rfbTranslateNone is used when no translation is required.

void
rfbTranslateNone(char *table, rfbPixelFormat *in, rfbPixelFormat *out,
		 char *iptr, char *optr, int bytesBetweenInputLines,
		 int width, int height)
{
    int bytesPerOutputLine = width * (out->bitsPerPixel >> 3);

    while (height > 0) {
	memcpy(optr, iptr, bytesPerOutputLine);
	iptr += bytesBetweenInputLines;
	optr += bytesPerOutputLine;
	--height;
    }
}



HDC GetDcMirror()
{
typedef BOOL (WINAPI* pEnumDisplayDevices)(PVOID,DWORD,PVOID,DWORD);
		HDC m_hrootdc=NULL;
		pEnumDisplayDevices pd;
		LPSTR driverName = "mv video hook driver2";
		BOOL DriverFound;
		DEVMODE devmode;
		FillMemory(&devmode, sizeof(DEVMODE), 0);
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmDriverExtra = 0;
		BOOL change = EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
		devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		HMODULE hUser32=LoadLibrary("USER32");
		pd = (pEnumDisplayDevices)GetProcAddress( hUser32, "EnumDisplayDevicesA");
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
				while (result = (*pd)(NULL,devNum, &dd,0))
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
				}
			}
		if (hUser32) FreeLibrary(hUser32);

		return m_hrootdc;
}