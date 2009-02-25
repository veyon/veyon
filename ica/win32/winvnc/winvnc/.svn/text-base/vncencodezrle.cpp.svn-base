//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
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
// which you received this file, check http://www.realvnc.com/ or contact
// the authors on info@realvnc.com for information on obtaining it.

#include "vncEncodeZRLE.h"
#include "rfb.h"
#include "rfbMisc.h"
#include <stdlib.h>
#include <time.h>
#include <rdr/MemOutStream.h>
#include <rdr/ZlibOutStream.h>

#define GET_IMAGE_INTO_BUF(tx,ty,tw,th,buf)     \
  rfb::Rect rect;                                    \
  rect.tl.x = tx;                               \
  rect.tl.y = ty;                                \
  rect.br.x = tx+tw;                           \
  rect.br.y = ty+th;                          \
  encoder->Translate(source, (BYTE*)buf, rect);

#define EXTRA_ARGS , BYTE* source, vncEncoder* encoder

#define ENDIAN_LITTLE 0
#define ENDIAN_BIG 1
#define ENDIAN_NO 2
#define BPP 8
#define ZYWRLE_ENDIAN ENDIAN_NO
#include <rfb/zrleEncode.h>
#undef BPP
#define BPP 15
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_LITTLE
#include <rfb/zrleEncode.h>
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_BIG
#include <rfb/zrleEncode.h>
#undef BPP
#define BPP 16
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_LITTLE
#include <rfb/zrleEncode.h>
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_BIG
#include <rfb/zrleEncode.h>
#undef BPP
#define BPP 32
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_LITTLE
#include <rfb/zrleEncode.h>
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_BIG
#include <rfb/zrleEncode.h>
#define CPIXEL 24A
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_LITTLE
#include <rfb/zrleEncode.h>
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_BIG
#include <rfb/zrleEncode.h>
#undef CPIXEL
#define CPIXEL 24B
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_LITTLE
#include <rfb/zrleEncode.h>
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_BIG
#include <rfb/zrleEncode.h>
#undef CPIXEL
#undef BPP

vncEncodeZRLE::vncEncodeZRLE()
{
  mos = new rdr::MemOutStream;
  zos = new rdr::ZlibOutStream;
  beforeBuf = new rdr::U32[rfbZRLETileWidth * rfbZRLETileHeight + 1];
  m_use_zywrle = FALSE;
}

vncEncodeZRLE::~vncEncodeZRLE()
{
  delete mos;
  delete zos;
  delete [] beforeBuf;
}

void vncEncodeZRLE::Init()
{
  vncEncoder::Init();
}

UINT vncEncodeZRLE::RequiredBuffSize(UINT width, UINT height)
{
  // this is a guess - 12 bytes plus 1.5 times raw... (zlib.h says compress
  // needs 12 bytes plus 1.001 times raw data but that's not quite what we give
  // zlib anyway)
  return (sz_rfbFramebufferUpdateRectHeader + sz_rfbZRLEHeader + 12 +
          width * height * (m_remoteformat.bitsPerPixel / 8) * 3 / 2);
}

UINT vncEncodeZRLE::EncodeRect(BYTE *source, BYTE *dest, const rfb::Rect &rect)
{
  int x = rect.tl.x;
  int y = rect.tl.y;
  int w = rect.br.x - x;
  int h = rect.br.y - y;

  mos->clear();

  if( m_use_zywrle ){
	  if( m_qualitylevel < 0 ){
		  zywrle_level = 1;
	  }else if( m_qualitylevel < 3 ){
		  zywrle_level = 3;
	  }else if( m_qualitylevel < 6 ){
		  zywrle_level = 2;
	  }else{
		  zywrle_level = 1;
	  }
  }else{
	  zywrle_level = 0;
  }

  switch (m_remoteformat.bitsPerPixel) {

  case 8:
    zrleEncode8NE( x, y, w, h, mos, zos, beforeBuf, source, this);
    break;

  case 16:
    if( m_remoteformat.greenMax > 0x1F ){
      if( m_remoteformat.bigEndian ){
        zrleEncode16BE(x, y, w, h, mos, zos, beforeBuf, source, this);
	  }else{
        zrleEncode16LE(x, y, w, h, mos, zos, beforeBuf, source, this);
	  }
	}else{
      if( m_remoteformat.bigEndian ){
        zrleEncode15BE(x, y, w, h, mos, zos, beforeBuf, source, this);
	  }else{
        zrleEncode15LE(x, y, w, h, mos, zos, beforeBuf, source, this);
	  }
	}
    break;

  case 32:
    bool fitsInLS3Bytes
      = ((m_remoteformat.redMax   << m_remoteformat.redShift)   < (1<<24) &&
         (m_remoteformat.greenMax << m_remoteformat.greenShift) < (1<<24) &&
         (m_remoteformat.blueMax  << m_remoteformat.blueShift)  < (1<<24));

    bool fitsInMS3Bytes = (m_remoteformat.redShift   > 7  &&
                           m_remoteformat.greenShift > 7  &&
                           m_remoteformat.blueShift  > 7);

    if ((fitsInLS3Bytes && !m_remoteformat.bigEndian) ||
        (fitsInMS3Bytes && m_remoteformat.bigEndian))
    {
      if( m_remoteformat.bigEndian ){
        zrleEncode24ABE(x, y, w, h, mos, zos, beforeBuf, source, this);
	  }else{
        zrleEncode24ALE(x, y, w, h, mos, zos, beforeBuf, source, this);
	  }
    }
    else if ((fitsInLS3Bytes && m_remoteformat.bigEndian) ||
             (fitsInMS3Bytes && !m_remoteformat.bigEndian))
    {
      if( m_remoteformat.bigEndian ){
        zrleEncode24BBE(x, y, w, h, mos, zos, beforeBuf, source, this);
	  }else{
        zrleEncode24BLE(x, y, w, h, mos, zos, beforeBuf, source, this);
	  }
    }
    else
    {
      if( m_remoteformat.bigEndian ){
        zrleEncode32BE(x, y, w, h, mos, zos, beforeBuf, source, this);
	  }else{
        zrleEncode32LE(x, y, w, h, mos, zos, beforeBuf, source, this);
	  }
    }
    break;
  }

  rfbFramebufferUpdateRectHeader* surh = (rfbFramebufferUpdateRectHeader*)dest;
  surh->r.x = Swap16IfLE(x-m_SWOffsetx);
  surh->r.y = Swap16IfLE(y-m_SWOffsety);
  surh->r.w = Swap16IfLE(w);
  surh->r.h = Swap16IfLE(h);
  if( m_use_zywrle ){
    surh->encoding = Swap32IfLE(rfbEncodingZYWRLE);
  }else{
    surh->encoding = Swap32IfLE(rfbEncodingZRLE);
  }

  rfbZRLEHeader* hdr = (rfbZRLEHeader*)(dest +
                                        sz_rfbFramebufferUpdateRectHeader);

  hdr->length = Swap32IfLE(mos->length());

  memcpy(dest + sz_rfbFramebufferUpdateRectHeader + sz_rfbZRLEHeader,
         (rdr::U8*)mos->data(), mos->length());

  return sz_rfbFramebufferUpdateRectHeader + sz_rfbZRLEHeader + mos->length();
}
