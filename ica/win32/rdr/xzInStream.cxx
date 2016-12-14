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

#include "xzInStream.h"
#include "Exception.h"
#include "assert.h"

using namespace rdr;

enum { DEFAULT_BUF_SIZE = 0x10000 };

xzInStream::xzInStream(int bufSize_)
  : underlying(0), bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_SIZE), offset(0),
    bytesIn(0), ls(NULL)
{
	ptr = end = start = new U8[bufSize];
}

xzInStream::~xzInStream()
{
  delete [] start;
  if (ls) {
	  lzma_end(ls);
	  delete ls;
  }
}

void xzInStream::ensure_stream_codec()
{
	if (ls) return;
	
	ls = new lzma_stream;

	memset(ls, 0, sizeof(lzma_stream));
	lzma_ret rc = LZMA_OK;

	rc = lzma_stream_decoder(ls, UINT64_MAX, LZMA_TELL_UNSUPPORTED_CHECK);
	if (rc != LZMA_OK) {
		fprintf (stderr, "lzma_stream_decoder error: %d\n", (int) rc);
		throw Exception("lzmaOutStream: lzma_stream_decoder failed");
		return;
	}
}

void xzInStream::setUnderlying(InStream* is, int bytesIn_)
{
  underlying = is;
  bytesIn = bytesIn_;
  ptr = end = start;
}

int xzInStream::pos()
{
  return offset + ptr - start;
}

void xzInStream::reset()
{
  ensure_stream_codec();

  ptr = end = start;
  if (!underlying) return;

  while (bytesIn > 0) {
    decompress();
    end = start; // throw away any data
  }
  underlying = 0;
}

int xzInStream::overrun(int itemSize, int nItems)
{
  ensure_stream_codec();

  if (itemSize > bufSize)
    throw Exception("xzInStream overrun: max itemSize exceeded");
  if (!underlying)
    throw Exception("xzInStream overrun: no underlying stream");

  if (end - ptr != 0)
    memmove(start, ptr, end - ptr);

  offset += ptr - start;
  end -= ptr - start;
  ptr = start;

  while (end - ptr < itemSize) {
    decompress();
  }

  if (itemSize * nItems > end - ptr)
    nItems = (end - ptr) / itemSize;

  return nItems;
}

// decompress() calls the decompressor once.  Note that this won't necessarily
// generate any output data - it may just consume some input data.

void xzInStream::decompress()
{
	ls->next_out = (U8*)end;
	ls->avail_out = start + bufSize - end;

	if (bytesIn > 0) {
		underlying->check(1);
	} else {
		assert(false);
	}
	ls->next_in = (U8*)underlying->getptr();
	ls->avail_in = underlying->getend() - underlying->getptr();
	if ((int)ls->avail_in > bytesIn)
		ls->avail_in = bytesIn;

	lzma_ret rc = lzma_code(ls, LZMA_RUN);
	if ((rc != LZMA_OK) && (rc != LZMA_STREAM_END)) {
		fprintf (stderr, "lzma_code decompress error: %d\n", (int) rc);
		throw Exception("lzmaOutStream: decompress failed");
	}

	bytesIn -= ls->next_in - underlying->getptr();
	end = ls->next_out;
	underlying->setptr(ls->next_in);
}
