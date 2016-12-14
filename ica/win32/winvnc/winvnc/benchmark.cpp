/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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
#include <winsock2.h>
#include "windows.h"
#include <stdio.h>
#include <string.h>
#include "vnclog.h"
bool G_USE_PIXEL=false;
extern VNCLog vnclog;
#define VNCLOG(s)	(__FILE__ " : " s)

struct _BMInfo {
		BOOL			truecolour;
		BITMAPINFO		bmi;
		// Colormap info - comes straight after BITMAPINFO - **HACK**
		RGBQUAD			cmap[256];
	} m_bminfo;

void testBench()
{
	HDC			m_hrootdc=NULL;
	HDC			m_hmemdc=NULL;
	HBITMAP		m_membitmap=NULL;
	HBITMAP		m_oldbitmap=NULL;
	void		*m_DIBbits=NULL;

	m_hrootdc = GetDC(NULL);
	if (m_hrootdc == NULL) {
		return ;
	}
	m_hmemdc = CreateCompatibleDC(m_hrootdc);
	if (m_hmemdc == NULL) {
		return ;
	}
	m_membitmap = CreateCompatibleBitmap(m_hrootdc,1,1);
	if (m_membitmap == NULL) {
		return ;
	}


	int result;
	memset(&m_bminfo, 0, sizeof(m_bminfo));
	m_bminfo.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bminfo.bmi.bmiHeader.biBitCount = 0;
	result = ::GetDIBits(m_hmemdc, m_membitmap, 0, 1, NULL, &m_bminfo.bmi, DIB_RGB_COLORS);
	if (result == 0) {
		return;
	}
	result = ::GetDIBits(m_hmemdc, m_membitmap,  0, 1, NULL, &m_bminfo.bmi, DIB_RGB_COLORS);
	if (result == 0) {
		return;
	}
	RECT m_bmrect;
	m_bmrect.left=0;
	m_bmrect.top=0;
	m_bmrect.right=GetDeviceCaps(m_hrootdc, HORZRES);
	m_bmrect.bottom=GetDeviceCaps(m_hrootdc, VERTRES);
	// Henceforth we want to use a top-down scanning representation
    m_bminfo.bmi.bmiHeader.biWidth = m_bmrect.right;
    m_bminfo.bmi.bmiHeader.biHeight = m_bmrect.bottom;
    m_bminfo.bmi.bmiHeader.biSizeImage = abs((m_bminfo.bmi.bmiHeader.biWidth *
				m_bminfo.bmi.bmiHeader.biHeight *
				m_bminfo.bmi.bmiHeader.biBitCount)/ 8);
	m_bminfo.bmi.bmiHeader.biHeight = - abs(m_bminfo.bmi.bmiHeader.biHeight);

	// Is the bitmap palette-based or truecolour?
	m_bminfo.truecolour = (GetDeviceCaps(m_hmemdc, RASTERCAPS) & RC_PALETTE) == 0;

	int m_bytesPerRow = m_bminfo.bmi.bmiHeader.biWidth * m_bminfo.bmi.bmiHeader.biBitCount / 8;
	int m_bytesPerPixel= m_bminfo.bmi.bmiHeader.biBitCount / 8;

	HBITMAP tempbitmap = CreateDIBSection(m_hmemdc, &m_bminfo.bmi, DIB_RGB_COLORS, &m_DIBbits, NULL, 0);
	if (tempbitmap == NULL) {
		m_DIBbits = NULL;
        tempbitmap = CreateCompatibleBitmap(m_hrootdc, m_bmrect.right, m_bmrect.bottom);
	    if (tempbitmap == NULL) {
		    return;
	    }
	}

	// Delete the old memory bitmap
	if (m_membitmap != NULL) {
		DeleteObject(m_membitmap);
		m_membitmap = NULL;
	}

	// Replace old membitmap with DIB section
	m_membitmap = tempbitmap;
	DWORD time1,time2;
///---------------------------------------
	{
	DWORD start= timeGetTime();
	
	{
	if ((m_oldbitmap = (HBITMAP) SelectObject(m_hmemdc, m_membitmap)) == NULL)
					return;
	BOOL blitok = BitBlt(m_hmemdc, 0, 0, m_bmrect.right, m_bmrect.bottom, m_hrootdc, 0, 0, CAPTUREBLT | SRCCOPY);
	SelectObject(m_hmemdc, m_oldbitmap);
	}
	COLORREF cr = 0;
	for (int xx=0;xx<m_bmrect.right/32;xx++)
		for (int yy=0;yy<m_bmrect.bottom/32;yy++)
		{
			unsigned int index = (m_bytesPerRow * yy) + (m_bytesPerPixel * xx);
				memcpy(&cr, ((char*)m_DIBbits)+index, m_bytesPerPixel);
		}


	DWORD stop= timeGetTime();
	time1=stop-start;
	}
	
///---------------------------------------
	{
	DWORD start= timeGetTime();
	
	{
	COLORREF cr = 0;
	for (int xx=0;xx<m_bmrect.right*m_bmrect.bottom/32/32/200;xx++)
		{
			cr=GetPixel(m_hrootdc, 1, 1);
		}
	}
	DWORD stop= timeGetTime();
	time2=(stop-start)*200;
	}
///---------------------------------------

	if (time2<time1) G_USE_PIXEL=true;
	else G_USE_PIXEL=false;

	vnclog.Print(9, VNCLOG("Blit time %i  Getpixeltime %i  Use getpixel= %i\n"), time1,time2,G_USE_PIXEL);

	if (m_hrootdc != NULL)
	{
		if (!ReleaseDC(NULL,m_hrootdc))
		m_hrootdc = NULL;
	}
	if (m_hmemdc != NULL)
	{
		// Release our device context
		if (!DeleteDC(m_hmemdc))
		{
		}
		m_hmemdc = NULL;
	}
	if (m_membitmap != NULL)
	{
		// Release the custom bitmap, if any
		if (!DeleteObject(m_membitmap))
		{
		}
		m_membitmap = NULL;
	}
}