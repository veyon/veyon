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


//
// xzOutStream streams to a compressed data stream (underlying), compressing
// with zlib on the fly.
//

#ifndef __RDR_xzOutStream_H__
#define __RDR_xzOutStream_H__

#include "OutStream.h"

#define LZMA_API_STATIC
#ifndef _VS2008
#include <stdint.h>
#endif
#include "../xz-5.2.1/src/liblzma/api/lzma.h"

namespace rdr {

  class xzOutStream : public OutStream {

  public:

    // adzm - 2010-07 - Custom compression level
    xzOutStream(OutStream* os=0, int bufSize=0);
    virtual ~xzOutStream();

	void SetCompressLevel(int compression);

    void setUnderlying(OutStream* os);
    void flush();
    int length();

  private:

    void ensure_stream_codec();

    int overrun(int itemSize, int nItems);

    OutStream* underlying;
    int bufSize;
    int offset;
	lzma_stream* ls;
	lzma_options_lzma ls_options;
    U8* start;
  };

} // end of namespace rdr

#endif
