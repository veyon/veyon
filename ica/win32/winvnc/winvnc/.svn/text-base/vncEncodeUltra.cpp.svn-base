//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
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


// vncEncodeUltra

// This file implements the vncEncoder-derived vncEncodeUltra class.
// This class overrides some vncEncoder functions to produce a bitmap
// to Ultra encoder.  Ultra is much more efficient than RAW format on
// most screen data and usually twice as efficient as hextile.  Of
// course, Ultra compression uses more CPU time on the server.
// However, over slower (64kbps or less) connections, the reduction
// in data transmitted usually outweighs the extra latency added
// while the server CPU performs the compression algorithms.

#include "vncEncodeUltra.h"

#define IN_LEN		(128*1024)
#define OUT_LEN		(IN_LEN + IN_LEN / 64 + 16 + 3)
#define HEAP_ALLOC(var,size) \
	lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]
static HEAP_ALLOC(wrkmem,LZO1X_1_MEM_COMPRESS);

vncEncodeUltra::vncEncodeUltra()
{
	m_buffer = NULL;
	m_Queuebuffer = NULL;
	m_QueueCompressedbuffer = NULL;
	m_bufflen = 0;
	m_Queuelen = 0;
	MaxQueuebufflen=128*1024;

	m_Queuebuffer = new BYTE [MaxQueuebufflen+1];
		if (m_Queuebuffer == NULL)
		{
			vnclog.Print(LL_INTINFO, VNCLOG("Memory error"));
		}
	m_QueueCompressedbuffer = new BYTE [MaxQueuebufflen+(MaxQueuebufflen/100)+8];
		if (m_Queuebuffer == NULL)
		{
			vnclog.Print(LL_INTINFO, VNCLOG("Memory error"));
		}
	lzo=false;
}

vncEncodeUltra::~vncEncodeUltra()
{
	if (m_buffer != NULL)
	{
		delete [] m_buffer;
		m_buffer = NULL;
	}

	if (m_Queuebuffer != NULL)
	{
		delete [] m_Queuebuffer;
		m_Queuebuffer = NULL;
	}

	if (m_QueueCompressedbuffer != NULL)
	{
		delete [] m_QueueCompressedbuffer;
		m_QueueCompressedbuffer = NULL;
	}

	vnclog.Print(LL_INTINFO, VNCLOG("Ultra  encoder stats: rawdata=%d  protocol=%d compressed=%d transmitted=%d\n"),dataSize, rectangleOverhead, encodedSize,transmittedSize);

	if (dataSize != 0) {
		vnclog.Print(LL_INTINFO, VNCLOG("Ultra  encoder efficiency: %.3f%%\n"),(double)((double)((dataSize - transmittedSize) * 100) / dataSize));
	}
}

void
vncEncodeUltra::Init()
{
	totalraw=0;
	encodedSize=0;
	rectangleOverhead=0;
	transmittedSize=0;
	dataSize=0;
	vncEncoder::Init();
	m_nNbRects=0;
	m_queueEnable = true; // sf@2003
}

UINT
vncEncodeUltra::RequiredBuffSize(UINT width, UINT height)
{
	int result;

	// The Ultra library specifies a maximum compressed size of
	// the raw size plus one percent plus 8 bytes.  We also need
	// to cover the Ultra header space.
	result = vncEncoder::RequiredBuffSize(width, height);
	result += result/ 64 + 16 + 3 + sz_rfbZlibHeader+sz_rfbFramebufferUpdateRectHeader;
	return result;
}

UINT
vncEncodeUltra::NumCodedRects(const rfb::Rect &rect)
{
	return 0;
	const int rectW = rect.br.x - rect.tl.x;
	const int rectH = rect.br.y - rect.tl.y;
	return (( rectH - 1 ) / ( Ultra_MAX_SIZE( rectW ) / rectW ) + 1 );
}

/*****************************************************************************
 *
 * Routines to implement Ultra Encoding (LZ+Huffman compression) by calling
 * the included Ultra library.
 */

// Encode the rectangle using Ultra compression
inline UINT
vncEncodeUltra::EncodeRect(BYTE *source, VSocket *outConn, BYTE *dest, const rfb::Rect &rect)
{
	
	int  totalSize = 0;
	int  partialSize = 0;
	int  maxLines;
	int  linesRemaining;
	RECT partialRect;
	const int rectW = rect.br.x - rect.tl.x;
	const int rectH = rect.br.y - rect.tl.y;

	partialRect.right = rect.br.x;
	partialRect.left = rect.tl.x;
	partialRect.top = rect.tl.y;
	partialRect.bottom = rect.br.y;

	/* WBB: For testing purposes only! */
	// vnclog.Print(LL_INTINFO, VNCLOG("rect.right=%d rect.left=%d rect.top=%d rect.bottom=%d\n"), rect.right, rect.left, rect.top, rect.bottom);

	if (rectW==0) return 0;
	if (rectH==0) return 0;
	maxLines = ( Ultra_MAX_SIZE(rectW) / rectW );
	linesRemaining = rectH;

	while ( linesRemaining > 0 ) {

		int linesToComp;

		if ( maxLines < linesRemaining )
			linesToComp = maxLines;
		else
			linesToComp = linesRemaining;

		partialRect.bottom = partialRect.top + linesToComp;

		/* WBB: For testing purposes only! */
		// vnclog.Print(LL_INTINFO, VNCLOG("partialRect.right=%d partialRect.left=%d partialRect.top=%d partialRect.bottom=%d\n"), partialRect.right, partialRect.left, partialRect.top, partialRect.bottom);

		partialSize = EncodeOneRect( source,dest, partialRect,outConn );
		totalSize += partialSize;

		linesRemaining -= linesToComp;
		partialRect.top += linesToComp;

		if (( linesRemaining > 0 ) &&
			( partialSize > 0 ))
		{
			// Send the encoded data
			outConn->SendExactQueue( (char *)dest, partialSize );
			transmittedSize += partialSize;
		}


	}
	transmittedSize += partialSize;

	return partialSize;

}

// Encode the rectangle using zlib compression
inline UINT
vncEncodeUltra::EncodeOneRect(BYTE *source, BYTE *dest, const RECT &rect,VSocket *outConn)
{
	const int rectW = rect.right - rect.left;
	const int rectH = rect.bottom - rect.top;
	const int rawDataSize = (rectW*rectH*m_remoteformat.bitsPerPixel / 8);
	const int maxCompSize = (rawDataSize + (rawDataSize/100) + 8);

	// Create the rectangle header
	rfbFramebufferUpdateRectHeader *surh=(rfbFramebufferUpdateRectHeader *)dest;
	// Modif rdv@2002 - v1.1.x - Application Resize
	surh->r.x = (CARD16) rect.left-m_SWOffsetx;
	surh->r.y = (CARD16) rect.top-m_SWOffsety;
	surh->r.w = (CARD16) (rectW);
	surh->r.h = (CARD16) (rectH);
	surh->r.x = Swap16IfLE(surh->r.x);
	surh->r.y = Swap16IfLE(surh->r.y);
	surh->r.w = Swap16IfLE(surh->r.w);
	surh->r.h = Swap16IfLE(surh->r.h);
	surh->encoding = Swap32IfLE(rfbEncodingUltra);

	dataSize += ( rectW * rectH * m_remoteformat.bitsPerPixel) / 8;
	rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;
	
	// create a space big enough for the Zlib encoded pixels
	if (m_bufflen < rawDataSize)
	{
		if (m_buffer != NULL)
		{
			delete [] m_buffer;
			m_buffer = NULL;
		}
		m_buffer = new BYTE [rawDataSize+1];
		if (m_buffer == NULL)
			return vncEncoder::EncodeRect(source, dest, rect);
		m_bufflen = rawDataSize;
	}
	// Translate the data into our new buffer
	Translate(source, m_buffer, rect);

	// Perhaps we can queue the small updates and compress them combined
	if (rawDataSize < VNC_ENCODE_ULTRA_MIN_COMP_SIZE)
	{
		if (m_queueEnable)
			{
				surh->encoding = Swap32IfLE(rfbEncodingRaw);
				memcpy(dest+sz_rfbFramebufferUpdateRectHeader,m_buffer,rawDataSize);
				AddToQueu(dest,sz_rfbFramebufferUpdateRectHeader +rawDataSize,outConn,0);
				return 0;
			}
		else return vncEncoder::EncodeRect(source, dest, rect);
	}

	
	if (rawDataSize<1000 && m_queueEnable)
		{
			surh->encoding = Swap32IfLE(rfbEncodingRaw);
			memcpy(dest+sz_rfbFramebufferUpdateRectHeader,m_buffer,rawDataSize);
			AddToQueu(dest,sz_rfbFramebufferUpdateRectHeader +rawDataSize,outConn,1);
			return 0;
		}

	surh->encoding = Swap32IfLE(rfbEncodingUltra);
				
	if (lzo==false)
		{
			if (lzo_init() == LZO_E_OK) lzo=true;
		}
	lzo1x_1_compress(m_buffer,rawDataSize,dest+sz_rfbFramebufferUpdateRectHeader+sz_rfbZlibHeader,&out_len,wrkmem);
	if (out_len>rawDataSize)
				{
					return vncEncoder::EncodeRect(source, dest, rect);
				}
	
		// Format the ZlibHeader
		rfbZlibHeader *zlibh=(rfbZlibHeader *)(dest+sz_rfbFramebufferUpdateRectHeader);
		zlibh->nBytes = Swap32IfLE(out_len);
	
		// Update statistics
		encodedSize += sz_rfbZlibHeader + out_len;
		rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;

		// Return the amount of data sent	
		return sz_rfbFramebufferUpdateRectHeader +
			sz_rfbZlibHeader +
			out_len;

}



void
vncEncodeUltra::AddToQueu(BYTE *source,int sizerect,VSocket *outConn,int updatetype)
{
	if (m_Queuelen+sizerect>(MaxQueuebufflen)) SendUltrarects(outConn);
	memcpy(m_Queuebuffer+m_Queuelen,source,sizerect);
	m_Queuelen+=sizerect;
	m_nNbRects++;
	if (updatetype==1) must_be_zipped=true;
	if (m_nNbRects>0) SendUltrarects(outConn);
}

void
vncEncodeUltra::AddToQueu2(BYTE *source,int sizerect,VSocket *outConn,int updatetype)
{
	BYTE *databegin=m_QueueCompressedbuffer+sz_rfbFramebufferUpdateRectHeader+sz_rfbZlibHeader;
	rfbFramebufferUpdateRectHeader *CacheRectsHeader=(rfbFramebufferUpdateRectHeader*)m_QueueCompressedbuffer;
	rfbZlibHeader *CacheZipHeader=(rfbZlibHeader*)m_QueueCompressedbuffer+sz_rfbFramebufferUpdateRectHeader;
	const int rawDataSize = (sizerect);
	lzo1x_1_compress(source,rawDataSize,databegin,&out_len,wrkmem);

	if (out_len>rawDataSize)
				{
					outConn->SendExactQueue( (char *)source, sizerect); // 1 Small update
					encodedSize += sizerect-sz_rfbFramebufferUpdateRectHeader;
					rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;
					return;
				}

	int rawDataSize1=rawDataSize/65535;
	int rawDataSize2=rawDataSize%65535;

	CacheRectsHeader->r.x = (CARD16)(1);
	CacheRectsHeader->r.y = (CARD16)(rawDataSize2);
	CacheRectsHeader->r.w = (CARD16)(rawDataSize1);
	CacheRectsHeader->r.x = Swap16IfLE(CacheRectsHeader->r.x);
	CacheRectsHeader->r.y = Swap16IfLE(CacheRectsHeader->r.y);
	CacheRectsHeader->r.w = Swap16IfLE(CacheRectsHeader->r.w);
 	CacheRectsHeader->r.h = 0;
	CacheRectsHeader->encoding = Swap32IfLE(rfbEncodingUltraZip);

	// Format the UltraHeader
	CacheZipHeader->nBytes = Swap32IfLE(out_len);

	vnclog.Print(LL_INTINFO, VNCLOG("********QUEUEQUEUE********** %d %d %d\r\n"),out_len,rawDataSize,1);
	outConn->SendExactQueue((char *)m_QueueCompressedbuffer, out_len+sz_rfbFramebufferUpdateRectHeader+sz_rfbZlibHeader);
	// Update statistics
	encodedSize += sz_rfbZlibHeader + out_len;
	rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;
	transmittedSize += out_len+sz_rfbFramebufferUpdateRectHeader+sz_rfbZlibHeader;
}

void
vncEncodeUltra::SendUltrarects(VSocket *outConn)
{
	int NRects=m_nNbRects;
	const int rawDataSize = (m_Queuelen);

	if (NRects==0) return; // NO update
	if (m_nNbRects<3 && !must_be_zipped) 
	{
		outConn->SendExactQueue( (char *)m_Queuebuffer, m_Queuelen); // 1 Small update
		m_nNbRects=0;
		m_Queuelen=0;
		encodedSize += m_Queuelen-sz_rfbFramebufferUpdateRectHeader;
		rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;
		return;
	}
	m_nNbRects=0;
	m_Queuelen=0;
	must_be_zipped=false;

	lzo1x_1_compress(m_Queuebuffer,rawDataSize,m_QueueCompressedbuffer,&out_len,wrkmem);

	if (out_len>rawDataSize)
				{
					outConn->SendExactQueue( (char *)m_Queuebuffer, m_Queuelen); // 1 Small update
					m_nNbRects=0;
					m_Queuelen=0;
					encodedSize += m_Queuelen-sz_rfbFramebufferUpdateRectHeader;
					rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;
					return;
				}

	int rawDataSize1=rawDataSize/65535;
	int rawDataSize2=rawDataSize%65535;

	rfbFramebufferUpdateRectHeader CacheRectsHeader;
	CacheRectsHeader.r.x = (CARD16)(NRects);
	CacheRectsHeader.r.y = (CARD16)(rawDataSize2);
	CacheRectsHeader.r.w = (CARD16)(rawDataSize1);
	CacheRectsHeader.r.x = Swap16IfLE(CacheRectsHeader.r.x);
	CacheRectsHeader.r.y = Swap16IfLE(CacheRectsHeader.r.y);
	CacheRectsHeader.r.w = Swap16IfLE(CacheRectsHeader.r.w);
 	CacheRectsHeader.r.h = 0;
	CacheRectsHeader.encoding = Swap32IfLE(rfbEncodingUltraZip);

	// Format the UltraHeader
	rfbZlibHeader CacheZipHeader;
	CacheZipHeader.nBytes = Swap32IfLE(out_len);

	vnclog.Print(LL_INTINFO, VNCLOG("********QUEUEQUEUE********** %d %d %d\r\n"),out_len,rawDataSize,NRects);
	outConn->SendExactQueue((char *)&CacheRectsHeader, sizeof(CacheRectsHeader));
	outConn->SendExactQueue((char *)&CacheZipHeader, sizeof(CacheZipHeader));
	outConn->SendExactQueue((char *)m_QueueCompressedbuffer, out_len);
	// Update statistics
	encodedSize += sz_rfbZlibHeader + out_len;
	rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;
	transmittedSize += out_len+sz_rfbFramebufferUpdateRectHeader+sz_rfbZlibHeader;
}

void
vncEncodeUltra::LastRect(VSocket *outConn)
{
	SendUltrarects(outConn);
}