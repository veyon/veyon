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


// vncEncodeUltra object

// The vncEncodeUltra object uses a Ultra based compression encoding to send rectangles
// to a client

class vncEncodeUltra;

#if !defined(_WINVNC_EncodeULTRA)
#define _WINVNC_EncodeULTRA
#pragma once

#include "vncEncoder.h"
#include "lzo/minilzo.h"

// Minimum Ultra rectangle size in bytes.  Anything smaller will
// not compress well due to overhead.
#define VNC_ENCODE_ULTRA_MIN_COMP_SIZE (32)

// Set maximum Ultra rectangle size in pixels.  Always allow at least
// two scan lines.
#define Ultra_MAX_RECT_SIZE (16*1024)
#define Ultra_MAX_SIZE(min) ((( min * 2 ) > Ultra_MAX_RECT_SIZE ) ? ( min * 2 ) : Ultra_MAX_RECT_SIZE )
#define SOLID_COLOR	0 // 1 color
#define MONO_COLOR	1 //2 colors
#define MULTI_COLOR	2 // >2 colors
#define PURE_Ultra	3 
#define	XOR_SEQUENCE 4 //XOR previous image


// Class definition

class vncEncodeUltra : public vncEncoder
{
// Fields
public:

// Methods
public:
	// Create/Destroy methods
	vncEncodeUltra();
	~vncEncodeUltra();

	virtual void Init();
	virtual const char* GetEncodingName() { return "Ultra"; }

	virtual UINT RequiredBuffSize(UINT width, UINT height);
	virtual UINT NumCodedRects(const rfb::Rect &rect);

	virtual UINT EncodeRect(BYTE *source,VSocket *outConn, BYTE *dest, const rfb::Rect &rect);
	virtual UINT EncodeOneRect(BYTE *source ,BYTE *dest, const RECT &rect,VSocket *outConn);

	virtual void LastRect(VSocket *outConn);
	virtual void AddToQueu(BYTE *source,int size,VSocket *outConn,int must_be_zipped);
	virtual void AddToQueu2(BYTE *source,int size,VSocket *outConn,int must_be_zipped);
	virtual void SendUltrarects(VSocket *outConn);

	void EnableQueuing(BOOL fEnable){ m_queueEnable = fEnable; };

// Implementation
protected:
	BYTE		      *m_buffer;
	BYTE			  *m_Queuebuffer;
	BYTE			  *m_QueueCompressedbuffer;
	int			       m_bufflen;
	int totalraw;
	int SoMoMu; //solid/mono/multi color

	int				   m_Queuebufflen;
	int				   MaxQueuebufflen;
	int				   m_Queuelen;
	int				   m_nNbRects;
	int				   must_be_zipped;
	BOOL				m_queueEnable;

	bool				lzo;
	lzo_uint out_len;
};

#endif // _WINVNC_EncodeUltra

