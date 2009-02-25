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
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.

// vncBuffer object

// The vncBuffer object provides a client-local copy of the screen
// It can tell the client which bits have changed in a given region
// It uses the specified vncDesktop to read screen data from
class vncDesktop;

class vncBuffer;

#if !defined(_WINVNC_VNCBUFFER)
#define _WINVNC_VNCBUFFER
#pragma once


// Includes

#include "stdhdrs.h"
#include "vncEncoder.h"
#include "rfbRegion.h"
#include "rfbRect.h"
#include "rfb.h"
#include "vncmemcpy.h"

// Class definition

class vncBuffer
{
// Methods
public:
	// Create/Destroy methods
	vncBuffer();
	~vncBuffer();

	BOOL SetDesktop(vncDesktop *desktop);

	// BUFFER INFO
	rfb::Rect GetSize();
	rfbPixelFormat GetLocalFormat();

	// BUFFER MANIPULATION
	BOOL CheckBuffer();

	// SCREEN SCANNING
//	void Clear(const rfb::Rect &rect);
	void CheckRegion(rfb::Region2D &dest,rfb::Region2D &cache, const rfb::Region2D &src);
	void CheckRect(rfb::Region2D &dest,rfb::Region2D &cache, const rfb::Rect &src);

	// SCREEN CAPTURE
	void CopyRect(const rfb::Rect &dest, const rfb::Point &delta);
	void GrabMouse();
	void GrabRegion(rfb::Region2D &src,BOOL driver);
	void GetMousePos(rfb::Rect &rect);

	// CACHE RDV
	void ClearCache();
	void ClearCacheRect(const rfb::Rect &dest);
	void ClearBack();
	void BlackBack();
	void EnableCache(BOOL enable);
	BOOL IsCacheEnabled();
	BOOL IsShapeCleared();
	void Display(int number);
	int GetDisplay();

	// sf@2005 - Grey Palette
	void EnableGreyPalette(BOOL enable);

	// Modif sf@2002 - Scaling
	void ScaleRect(rfb::Rect &rect);
	void GreyScaleRect(rfb::Rect &rect);
	rfb::Rect GetViewerSize();
	UINT GetScale();
	BOOL SetScale(int scale);

	// Modif sf@2002 - Optim
	BOOL SetAccuracy(int accuracy);

	// CURSOR HANDLING
	BOOL IsCursorUpdatePending(){return m_cursorpending;};
	void SetCursorPending(BOOL enable){m_cursorpending=enable;};

	bool ClipRect(int *x, int *y, int *w, int *h, int cx, int cy, int cw, int ch);

// Implementation
protected:

	// Routine to verify the mainbuff handle hasn't changed
	BOOL FastCheckMainbuffer();
	
	// Fetch pixel data to the main buffer from the screen
	void GrabRect(const rfb::Rect &rect,BOOL driver);

	BOOL		m_freemainbuff;

	UINT		m_bytesPerRow;

	rfbServerInitMsg	m_scrinfo;

	// CACHE RDV
	BOOL			m_use_cache;
	BOOL			m_display_prim;
	BOOL			m_display_sec;

	// Modif sf@2002 - Scaling
	UINT		m_ScaledSize;
	UINT		m_nScale;
	BYTE		*m_ScaledBuff;

	int			m_nAccuracyDiv; // Accuracy divider for changes detection in Rects
	int			nRowIndex;

	// CURSOR HANDLING
	BOOL			m_cursorpending;

public:
	// vncEncodeMgr reads data from back buffer directly when encoding
	BYTE		*m_backbuff;
	UINT		m_backbuffsize;
	// CACHE RDV
	BYTE		*m_cachebuff;
	BYTE		*m_mainbuff;
	vncDesktop	*m_desktop;
//	Ultravncmemcpy mem;
	BOOL			m_cursor_shape_cleared;

	// sf@2005 - Grey palette
	BOOL m_fGreyPalette;
};

#endif // _WINVNC_VNCBUFFER
