//
// Copyright (C) 2002 RealVNC Ltd.  All Rights Reserved.
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
// USA.

#include "xzOutStream.h"
#include "Exception.h"

using namespace rdr;

enum { DEFAULT_BUF_SIZE = 0x10000 };

xzOutStream::xzOutStream(OutStream* os, int bufSize_)
  : underlying(os), bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_SIZE), offset(0), ls(NULL)
{
	SetCompressLevel(6);

	ptr = start = new U8[bufSize];
	end = start + bufSize;
}

xzOutStream::~xzOutStream()
{
  try {
    flush();
  } catch (Exception&) {
  }
  delete [] start;

  if (ls) {
	  lzma_end(ls);
	  delete ls;
  }
}

void xzOutStream::SetCompressLevel(int compression)
{
	if (ls) return;
	
	/*
	if (lzma_lzma_preset(&ls_options, compressionLevel)) {
		fprintf (stderr, "lzma_lzma_preset error\n");
		throw Exception("xzOutStream: lzma_lzma_preset failed");
		return;
	}
	*/

	memset(&ls_options, 0, sizeof(ls_options));
	ls_options.preset_dict = NULL;
	ls_options.preset_dict_size = 0;
	
	// default
	ls_options.mode = LZMA_MODE_NORMAL;
	ls_options.depth = 0;
	ls_options.dict_size = 0x00200000; //22
	ls_options.mf = LZMA_MF_HC4;
	ls_options.nice_len = 0x80;
	ls_options.lc = 2; // 2 high bits
	ls_options.lp = 0;
	ls_options.pb = 0; // 1 byte
	
	uint8_t dictionary_sizes[] = {
		18,			// 0		256kb
		19,			// 1		512kb
		20, 		// 2		1mb
		21, 		// 3		2mb
		21, 		// 4		
		22, 		// 5		4mb
		22, 		// 6		
		23, 		// 7		8mb
		23, 		// 8		
		24			// 9		16mb
	};

	ls_options.dict_size = uint32_t(1) << dictionary_sizes[compression];
		

	if (compression <= 3) {
		ls_options.mode = LZMA_MODE_FAST;
	}

	if (compression >= 6) {
		ls_options.nice_len = 0xC0;
	}

	if (compression >= 8) {
		ls_options.nice_len = 0x111;
		ls_options.depth = 0x200;
	}
}

void xzOutStream::ensure_stream_codec()
{
	if (ls) return;	

	ls = new lzma_stream;

	memset(ls, 0, sizeof(lzma_stream));
	lzma_ret rc = LZMA_OK;

	lzma_filter filters[2] = 
	{
		{LZMA_FILTER_LZMA2, &ls_options},
		{LZMA_VLI_UNKNOWN, NULL} //LZMA_VLI_UNKNOWN Lables End Of Filter List;
	};

	rc = lzma_stream_encoder(ls, filters, LZMA_CHECK_NONE);
	if (rc != LZMA_OK) {
		fprintf (stderr, "lzma_stream_encoder error: %d\n", (int) rc);
		throw Exception("xzOutStream: lzma_stream_encoder failed");
		return;
	}
}

void xzOutStream::setUnderlying(OutStream* os)
{
  underlying = os;
}

int xzOutStream::length()
{
  return offset + ptr - start;
}

void xzOutStream::flush()
{
	ensure_stream_codec();
	
	ls->next_in = start;
	ls->avail_in = ptr - start;

	//    fprintf(stderr,"zos flush: avail_in %d\n",zs->avail_in);

	while (ls->avail_in != 0) {

		do {
			underlying->check(1);
			ls->next_out = underlying->getptr();
			ls->avail_out = underlying->getend() - underlying->getptr();

			//        fprintf(stderr,"zos flush: calling deflate, avail_in %d, avail_out %d\n",
			//                zs->avail_in,zs->avail_out);
			lzma_ret rc = lzma_code(ls, LZMA_SYNC_FLUSH);
			if ((rc != LZMA_OK) && (rc != LZMA_STREAM_END)) {
				fprintf (stderr, "lzma_code flush error: %d\n", (int) rc);
				throw Exception("xzOutStream: compress flush failed");
			}

			//        fprintf(stderr,"zos flush: after deflate: %d bytes\n",
			//                zs->next_out-underlying->getptr());

			underlying->setptr(ls->next_out);
		} while (ls->avail_out == 0);
	}
	
	offset += ptr - start;
	ptr = start;
}

int xzOutStream::overrun(int itemSize, int nItems)
{
	//    fprintf(stderr,"xzOutStream overrun\n");

	ensure_stream_codec();

	if (itemSize > bufSize)
		throw Exception("xzOutStream overrun: max itemSize exceeded");

	
	while (end - ptr < itemSize) {
		ls->next_in = start;
		ls->avail_in = ptr - start;

		do {
			underlying->check(1);
			ls->next_out = underlying->getptr();
			ls->avail_out = underlying->getend() - underlying->getptr();

			//        fprintf(stderr,"zos overrun: calling deflate, avail_in %d, avail_out %d\n",
			//                zs->avail_in,zs->avail_out);

			lzma_ret rc = lzma_code(ls, LZMA_RUN);
			if ((rc != LZMA_OK) && (rc != LZMA_STREAM_END)) {
				fprintf (stderr, "lzma_code error: %d\n", (int) rc);
				throw Exception("xzOutStream: compress failed");
			}

			//        fprintf(stderr,"zos overrun: after deflate: %d bytes\n",
			//                zs->next_out-underlying->getptr());

			underlying->setptr(ls->next_out);
		} while (ls->avail_out == 0);

		// output buffer not full

		if (ls->avail_in == 0) {
			offset += ptr - start;
			ptr = start;
		} else {
			// but didn't consume all the data?  try shifting what's left to the
			// start of the buffer.
			fprintf(stderr,"lzma out buf not full, but in data not consumed\n");
			memmove(start, ls->next_in, ptr - ls->next_in);
			offset += ls->next_in - start;
			ptr -= ls->next_in - start;
		}
	}
	
	if (itemSize * nItems > end - ptr)
		nItems = (end - ptr) / itemSize;

	return nItems;
}
