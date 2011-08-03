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


// vncEncodeZlib

// This file implements the vncEncoder-derived vncEncodeZlib class.
// This class overrides some vncEncoder functions to produce a bitmap
// to Zlib encoder.  Zlib is much more efficient than RAW format on
// most screen data and usually twice as efficient as hextile.  Of
// course, zlib compression uses more CPU time on the server.
// However, over slower (64kbps or less) connections, the reduction
// in data transmitted usually outweighs the extra latency added
// while the server CPU performs the compression algorithms.

#include "vncEncodeZlib.h"

vncEncodeZlib::vncEncodeZlib()
{
	m_buffer = NULL;
	m_buffer2 = NULL;
	m_Queuebuffer = NULL;
	m_QueueCompressedbuffer = NULL;
	m_bufflen = 0;
	m_Queuelen = 0;
	m_Maskbuffer =NULL;
	m_MaskbufferSize =0;
	compStreamInited = false;
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
}

vncEncodeZlib::~vncEncodeZlib()
{
	if (m_buffer != NULL)
	{
		delete [] m_buffer;
		m_buffer = NULL;
	}

	if (m_buffer2 != NULL)
	{
		delete [] m_buffer2;
		m_buffer2 = NULL;
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

	if (m_Maskbuffer !=NULL)
	{
		delete [] m_Maskbuffer;
		m_Maskbuffer = NULL;

	}

	if ( compStreamInited == true )
	{
		deflateEnd( &compStream );
	}
	compStreamInited = false;

	vnclog.Print(LL_INTINFO, VNCLOG("Zlib Xor encoder stats: rawdata=%d  protocol=%d compressed=%d transmitted=%d\n"),dataSize, rectangleOverhead, encodedSize,transmittedSize);

	if (dataSize != 0) {
		vnclog.Print(LL_INTINFO, VNCLOG("Zlib Xor encoder efficiency: %.3f%%\n"),(double)((double)((dataSize - transmittedSize) * 100) / dataSize));
	}
}

void
vncEncodeZlib::Init()
{
	totalraw=0;
	encodedSize=0;
	rectangleOverhead=0;
	transmittedSize=0;
	dataSize=0;
	vncEncoder::Init();
	m_nNbRects=0;
}

UINT
vncEncodeZlib::RequiredBuffSize(UINT width, UINT height)
{
	int result;

	// The zlib library specifies a maximum compressed size of
	// the raw size plus one percent plus 8 bytes.  We also need
	// to cover the zlib header space.
	result = vncEncoder::RequiredBuffSize(width, height);
	Firstrun=result*2;//Needed to exclude xor when cachebuffer is empty
	result += ((result / 100) + 8) + sz_rfbZlibHeader;
	return result;
}

UINT
vncEncodeZlib::NumCodedRects(const rfb::Rect &rect)
{
	const int rectW = rect.br.x - rect.tl.x;
	const int rectH = rect.br.y - rect.tl.y;
	int aantal=(( rectH - 1 ) / ( ZLIB_MAX_SIZE( rectW ) / rectW ) + 1 );
	m_queueEnable=false;
	if (m_use_lastrect && aantal>1) {
		m_queueEnable=true;
		return 0;
	}
/******************************************************************
	return 1;
******************************************************************/

	// Return the number of rectangles needed to encode the given
	// update.  ( ZLIB_MAX_SIZE(rectW) / rectW ) is the number of lines in 
	// each maximum size rectangle.
	// When solid is enabled, most of the pixels are removed
	return (( rectH - 1 ) / ( ZLIB_MAX_SIZE( rectW ) / rectW ) + 1 );
}

/*****************************************************************************
 *
 * Routines to implement zlib Encoding (LZ+Huffman compression) by calling
 * the included zlib library.
 */

// Encode the rectangle using zlib compression
inline UINT
vncEncodeZlib::EncodeRect(BYTE *source,BYTE *source2, VSocket *outConn, BYTE *dest, const rfb::Rect &rect)
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

	maxLines = ( ZLIB_MAX_SIZE(rectW) / rectW );
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

		partialSize = EncodeOneRect( source,source2, dest, partialRect,outConn );
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
vncEncodeZlib::EncodeOneRect(BYTE *source,BYTE *source2, BYTE *dest, const RECT &rect,VSocket *outConn)
{
	int totalCompDataLen = 0;
	int previousTotalOut;
	int deflateResult;

	const int rectW = rect.right - rect.left;
	const int rectH = rect.bottom - rect.top;
	const int rawDataSize = (rectW*rectH*m_remoteformat.bitsPerPixel / 8);
	const int maxCompSize = (rawDataSize + (rawDataSize/100) + 8);

	if (!outConn->m_pIntegratedPluginInterface) m_queueEnable=false;

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
	surh->encoding = Swap32IfLE(rfbEncodingZlib);

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
		if (m_buffer2 != NULL)
		{
			delete [] m_buffer2;
			m_buffer2 = NULL;
		}
		m_buffer2 = new BYTE [rawDataSize+1];
	}
	// Translate the data into our new buffer
	compStream.avail_in = rawDataSize;
	Translate(source, m_buffer, rect);
	// Perhaps we can queue the small updates and compress them combined
	if (rawDataSize < VNC_ENCODE_ZLIB_MIN_COMP_SIZE)
	{
		if (m_queueEnable && source2 && dataSize>Firstrun)
			{
				surh->encoding = Swap32IfLE(rfbEncodingRaw);
				memcpy(dest+sz_rfbFramebufferUpdateRectHeader,m_buffer,rawDataSize);
				AddToQueu(dest,sz_rfbFramebufferUpdateRectHeader +rawDataSize,outConn,0);
				return 0;
			}
		else return vncEncoder::EncodeRect(source, dest, rect);
	}


	UINT newsize;
	SoMoMu=PURE_ZLIB;
	if (m_buffer2 && source2 && dataSize>Firstrun)
		{
			Translate(source2,m_buffer2, rect);
			newsize=PrepareXOR(m_buffer,m_buffer2,rect);
		}
	switch (SoMoMu)
	{
		case SOLID_COLOR:
			{
				surh->encoding = Swap32IfLE(rfbEncodingSolidColor);
				memcpy(dest+sz_rfbFramebufferUpdateRectHeader,m_buffer,compStream.avail_in);
				//vnclog.Print(LL_INTINFO, VNCLOG("Solid \n"));
				if (m_queueEnable)
				{
				AddToQueu(dest,sz_rfbFramebufferUpdateRectHeader +newsize,outConn,0);
				return 0;
				}
				return sz_rfbFramebufferUpdateRectHeader +newsize;
			}
		case MONO_COLOR:
			{
				compStream.avail_in = newsize;
				surh->encoding = Swap32IfLE(rfbEncodingXORMonoColor_Zlib);
				//vnclog.Print(LL_INTINFO, VNCLOG("Mono \n"));
				if (m_queueEnable)
				{
				memcpy(dest+sz_rfbFramebufferUpdateRectHeader,m_buffer,newsize);
				AddToQueu(dest,sz_rfbFramebufferUpdateRectHeader +newsize,outConn,1);
				return 0;
				}
				break;

			}
		case MULTI_COLOR:
			{
				compStream.avail_in = newsize;
				surh->encoding = Swap32IfLE(rfbEncodingXORMultiColor_Zlib);
				///vnclog.Print(LL_INTINFO, VNCLOG("MultiColor \n"));
				break;
			}

		case XOR_SEQUENCE:
			{
				compStream.avail_in = newsize;
				surh->encoding = Swap32IfLE(rfbEncodingXOR_Zlib);
				memcpy(dest+sz_rfbFramebufferUpdateRectHeader,m_buffer,newsize);
				if (newsize<1000 && m_queueEnable)
				{
					AddToQueu(dest,sz_rfbFramebufferUpdateRectHeader +newsize,outConn,1);
					return 0;
				}
				///vnclog.Print(LL_INTINFO, VNCLOG("XOR \n"));
				break;
			}

		case PURE_ZLIB:
			{
				if (rawDataSize<1000 && m_queueEnable)
				{
				surh->encoding = Swap32IfLE(rfbEncodingRaw);
				memcpy(dest+sz_rfbFramebufferUpdateRectHeader,m_buffer,rawDataSize);
				AddToQueu(dest,sz_rfbFramebufferUpdateRectHeader +rawDataSize,outConn,1);
				return 0;
				}

				surh->encoding = Swap32IfLE(rfbEncodingZlib);
				///vnclog.Print(LL_INTINFO, VNCLOG("Pure \n"));
				break;
			}
	}
		// Initialize input/output buffer assignment for compressor state.
		compStream.next_in = m_buffer;
		compStream.avail_out = maxCompSize;
		compStream.next_out = (dest+sz_rfbFramebufferUpdateRectHeader+sz_rfbZlibHeader);
		compStream.data_type = Z_BINARY;
	
		// If necessary, the first time, initialize the compressor state.
		if ( compStreamInited == false )
		{
	
			compStream.total_in = 0;
			compStream.total_out = 0;
			compStream.zalloc = Z_NULL;
			compStream.zfree = Z_NULL;
			compStream.opaque = Z_NULL;
	
			//vnclog.Print(LL_INTINFO, VNCLOG("calling deflateInit2 with zlib level:%d\n"), m_compresslevel);
	
			deflateResult = deflateInit2( &compStream,
										m_compresslevel,
										Z_DEFLATED,
										MAX_WBITS,
										MAX_MEM_LEVEL,
										Z_DEFAULT_STRATEGY );
			if ( deflateResult != Z_OK )
			{
				vnclog.Print(LL_INTINFO, VNCLOG("deflateInit2 returned error:%d:%s\n"), deflateResult, compStream.msg);
				return vncEncoder::EncodeRect(source, dest, rect);
			}
			compStreamInited = true;
		}

		// Record previous total output size.
		previousTotalOut = compStream.total_out;

		// Compress the raw data into the result buffer.
		deflateResult = deflate( &compStream, Z_SYNC_FLUSH );

		if ( deflateResult != Z_OK )
		{
			vnclog.Print(LL_INTINFO, VNCLOG("deflate returned error:%d:%s\n"), deflateResult, compStream.msg);
			return vncEncoder::EncodeRect(source, dest, rect);
		}
	
		// Calculate size of compressed data.
		totalCompDataLen = compStream.total_out - previousTotalOut;
	
		// Format the ZlibHeader
		rfbZlibHeader *zlibh=(rfbZlibHeader *)(dest+sz_rfbFramebufferUpdateRectHeader);
		zlibh->nBytes = Swap32IfLE(totalCompDataLen);
	
		// Update statistics
		encodedSize += sz_rfbZlibHeader + totalCompDataLen;
		rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;

		// Return the amount of data sent	
		return sz_rfbFramebufferUpdateRectHeader +
			sz_rfbZlibHeader +
			totalCompDataLen;

}

inline UINT
vncEncodeZlib::PrepareXOR(BYTE * m_buffer,BYTE * m_buffer2,RECT rect)
{
	
	DWORD c1;
	DWORD c2;
	unsigned char *c1ptr;
	unsigned char *c2ptr;

	unsigned char *sourceptr = m_buffer;
	unsigned char *source2ptr = m_buffer2;
	unsigned char *sourceposptr = m_buffer2;

	const int rectW = rect.right - rect.left;
	const int rectH = rect.bottom - rect.top;
	const int BytePerPixel=m_remoteformat.bitsPerPixel/8;
	const int rawDataSize = (rectW*rectH*BytePerPixel);
	const int maskDataSize = (((rectW*rectH)+7)/8);
	
	if (m_MaskbufferSize < maskDataSize)
	{
		if (m_Maskbuffer != NULL)
		{
			delete [] m_Maskbuffer;
			m_Maskbuffer = NULL;
		}
		m_Maskbuffer = new mybool [maskDataSize+1];
	}
	
	c1ptr=(unsigned char *)&c1;
	c2ptr=(unsigned char *)&c2;

	totalraw+=rawDataSize; //stats

	memcpy(c1ptr,sourceptr,BytePerPixel);
	// detect mono and solid parts
	int j=0;
	int nr_colors=1;
	bool temparray[8];
	int byte_nr=0;
	for (int i=0; i<rawDataSize;i=i+BytePerPixel)
	{
		if (memcmp(sourceptr,c1ptr,BytePerPixel)!= 0)
		{
			if (nr_colors==1)
				{
					memcpy(c2ptr,sourceptr,BytePerPixel);
					nr_colors++;
					SoMoMu=MONO_COLOR;
				}
			else if (memcmp(sourceptr,c2ptr,BytePerPixel)!= 0)
				{
						SoMoMu=MULTI_COLOR;
						nr_colors++;
						break;//stop checking we have more then 2 colors
				}
			temparray[j]=true;
		}
		else
		{
			temparray[j]=false;
		}
		if (j==7) //0-7 byte
		{
			m_Maskbuffer[byte_nr].b0=temparray[0];
			m_Maskbuffer[byte_nr].b1=temparray[1];
			m_Maskbuffer[byte_nr].b2=temparray[2];
			m_Maskbuffer[byte_nr].b3=temparray[3];
			m_Maskbuffer[byte_nr].b4=temparray[4];
			m_Maskbuffer[byte_nr].b5=temparray[5];
			m_Maskbuffer[byte_nr].b6=temparray[6];
			m_Maskbuffer[byte_nr].b7=temparray[7];
			byte_nr++;
			j=0;
		}
		else j++;
		sourceptr+=BytePerPixel;
		source2ptr+=BytePerPixel;
	}
	// add the last partial byte
	if (j!=0)
	{
		m_Maskbuffer[byte_nr].b0=temparray[0];
		m_Maskbuffer[byte_nr].b1=temparray[1];
		m_Maskbuffer[byte_nr].b2=temparray[2];
		m_Maskbuffer[byte_nr].b3=temparray[3];
		m_Maskbuffer[byte_nr].b4=temparray[4];
		m_Maskbuffer[byte_nr].b5=temparray[5];
		m_Maskbuffer[byte_nr].b6=temparray[6];
		m_Maskbuffer[byte_nr].b7=temparray[7];
	}

//SOLID COLOR
	if (nr_colors==1)
	{
	//full solid rectangle
		unsigned char *sourceptr = m_buffer;
		memcpy(sourceptr,c1ptr,BytePerPixel);
		SoMoMu=SOLID_COLOR;
		return BytePerPixel;
		}
//MONO COLOR
	if (nr_colors==2)
	{

		unsigned char *sourceptr = m_buffer;
		memcpy(sourceptr,m_Maskbuffer,maskDataSize);
		sourceptr+=maskDataSize;
		memcpy(sourceptr,c1ptr,BytePerPixel);
		sourceptr+=BytePerPixel;
		memcpy(sourceptr,c2ptr,BytePerPixel);
		SoMoMu=MONO_COLOR;
		return maskDataSize+(2)*BytePerPixel;
	}

//USE XOR AGAINST PREVIOUS IMAGE
	{
		int j=0;
		int k=0;
		SoMoMu=XOR_SEQUENCE;
		bool temparray[8];
		int byte_nr=0;
		sourceptr = m_buffer;
		source2ptr = m_buffer2;
		for (int i=0; i<rawDataSize;i=i+BytePerPixel)
		{
			if (memcmp(sourceptr,source2ptr,BytePerPixel)!= 0)
			{
				memcpy(sourceposptr,sourceptr,BytePerPixel);
				temparray[j]=true;
				sourceposptr+=BytePerPixel;
				k++;
			}
			else
			{
				temparray[j]=false;
			}
			if (j==7) //0-7 byte
			{
				m_Maskbuffer[byte_nr].b0=temparray[0];
				m_Maskbuffer[byte_nr].b1=temparray[1];
				m_Maskbuffer[byte_nr].b2=temparray[2];
				m_Maskbuffer[byte_nr].b3=temparray[3];
				m_Maskbuffer[byte_nr].b4=temparray[4];
				m_Maskbuffer[byte_nr].b5=temparray[5];
				m_Maskbuffer[byte_nr].b6=temparray[6];
				m_Maskbuffer[byte_nr].b7=temparray[7];
				byte_nr++;
				j=0;
			}
			else j++;
			sourceptr+=BytePerPixel;
			source2ptr+=BytePerPixel;
		}
		//add last byte, perhaps j==0
		if (j!=0)
		{
			m_Maskbuffer[byte_nr].b0=temparray[0];
			m_Maskbuffer[byte_nr].b1=temparray[1];
			m_Maskbuffer[byte_nr].b2=temparray[2];
			m_Maskbuffer[byte_nr].b3=temparray[3];
			m_Maskbuffer[byte_nr].b4=temparray[4];
			m_Maskbuffer[byte_nr].b5=temparray[5];
			m_Maskbuffer[byte_nr].b6=temparray[6];
			m_Maskbuffer[byte_nr].b7=temparray[7];
		}
	
		//check if new size is better then old
		if ((maskDataSize+(k)*BytePerPixel)<rawDataSize)
		{
			unsigned char *sourceptr = m_buffer;
			memcpy(sourceptr,m_Maskbuffer,maskDataSize);
			sourceptr+=maskDataSize;
			memcpy(sourceptr,m_buffer2,k*BytePerPixel);
			return maskDataSize+(k)*BytePerPixel;
		}
	}
	SoMoMu=PURE_ZLIB;
	return 0;
}

void
vncEncodeZlib::AddToQueu(BYTE *source,int sizerect,VSocket *outConn,int updatetype)
{
	if (m_Queuelen+sizerect>(MaxQueuebufflen)) SendZlibrects(outConn);
	memcpy(m_Queuebuffer+m_Queuelen,source,sizerect);
	m_Queuelen+=sizerect;
	m_nNbRects++;
	if (updatetype==1) must_be_zipped=true;
	if (m_nNbRects>50) SendZlibrects(outConn);
}

void
vncEncodeZlib::SendZlibrects(VSocket *outConn)
{
	int NRects=m_nNbRects;
	const int rawDataSize = (m_Queuelen);
	const int maxCompSize = (m_Queuelen + (m_Queuelen/100) + 8);

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


	int nRet = compress((unsigned char*)(m_QueueCompressedbuffer),
						(unsigned long*)&maxCompSize,
						(unsigned char*)m_Queuebuffer,
						rawDataSize
						);

	if (nRet != 0)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("compression error"));
		return ;
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
	CacheRectsHeader.encoding = Swap32IfLE(rfbEncodingSolMonoZip);

	// Format the ZlibHeader
	rfbZlibHeader CacheZipHeader;
	CacheZipHeader.nBytes = Swap32IfLE(maxCompSize);

	vnclog.Print(LL_INTINFO, VNCLOG("********QUEUEQUEUE********** %d %d %d\r\n"),maxCompSize,rawDataSize,NRects);
	outConn->SendExactQueue((char *)&CacheRectsHeader, sizeof(CacheRectsHeader));
	outConn->SendExactQueue((char *)&CacheZipHeader, sizeof(CacheZipHeader));
	outConn->SendExactQueue((char *)m_QueueCompressedbuffer, maxCompSize);
	// Update statistics
	encodedSize += sz_rfbZlibHeader + maxCompSize;
	rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;
	transmittedSize += maxCompSize+sz_rfbFramebufferUpdateRectHeader+sz_rfbZlibHeader;
}

void
vncEncodeZlib::LastRect(VSocket *outConn)
{
	SendZlibrects(outConn);
}
