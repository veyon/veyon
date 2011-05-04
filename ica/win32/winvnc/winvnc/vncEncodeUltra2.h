//  Copyright (C) 2000 Tridia Corporation. All Rights Reserved.
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
// TightVNC distribution homepage on the Web: http://www.tightvnc.com/
//
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// vncEncodeUltra2 object

// The vncEncodeUltra2 object uses a Ultra based compression encoding to send rectangles
// to a client

class vncEncodeUltra2;

#if !defined(_WINVNC_EncodeULTRA2)
#define _WINVNC_EncodeULTRA2
#pragma once
#include "vncencoder.h"
#include "lzo/minilzo.h"
#include "libjpeg-turbo-win/jpeglib.h"

// Class definition

class vncEncodeUltra2 : public vncEncoder
{
// Fields
public:

// Methods
public:
	// Create/Destroy methods
	vncEncodeUltra2();
	~vncEncodeUltra2();

	virtual void Init();
	virtual const char* GetEncodingName() { return "Ultra"; }

	virtual UINT RequiredBuffSize(UINT width, UINT height);
	virtual UINT NumCodedRects(const rfb::Rect &rect);

	virtual UINT EncodeRect(BYTE *source,VSocket *outConn, BYTE *dest, const rfb::Rect &rect);

// Implementation
public:
	BYTE		      *m_buffer;
	int			       m_bufflen;
	int SendJpegRect(BYTE *src,BYTE *dst, int dst_size, int w, int h, int quality,rfbPixelFormat m_remoteformat);
	bool				lzo;
	lzo_uint out_len;
	unsigned char *destbuffer;
};

#endif // _WINVNC_EncodeUltra

