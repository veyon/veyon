/*
 *  Copyright (C) 2005 Johannes E. Schindelin.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#ifdef LIBVNCSERVER_HAVE_LIBZ

/*
 * zrle.c - handle zrle encoding.
 *
 * This file shouldn't be compiled directly.  It is included multiple times by
 * rfbproto.c, each time with a different definition of the macro BPP.  For
 * each value of BPP, this file defines a function which handles an zrle
 * encoded rectangle with BPP bits per pixel.
 */

#ifndef REALBPP
#define REALBPP BPP
#endif

#if !defined(UNCOMP) || UNCOMP==0
#define HandleZRLE CONCAT2E(HandleZRLE,REALBPP)
#define HandleZRLETile CONCAT2E(HandleZRLETile,REALBPP)
#elif UNCOMP>0
#define HandleZRLE CONCAT3E(HandleZRLE,REALBPP,Down)
#define HandleZRLETile CONCAT3E(HandleZRLETile,REALBPP,Down)
#else
#define HandleZRLE CONCAT3E(HandleZRLE,REALBPP,Up)
#define HandleZRLETile CONCAT3E(HandleZRLETile,REALBPP,Up)
#endif
#define CARDBPP CONCAT3E(uint,BPP,_t)
#define CARDREALBPP CONCAT3E(uint,REALBPP,_t)

static int HandleZRLETile(rfbClient* client,
	uint8_t* buffer,size_t buffer_length,
	int x,int y,int w,int h);

static rfbBool
HandleZRLE (rfbClient* client, int rx, int ry, int rw, int rh)
{
	rfbZRLEHeader header;
	int remaining;
	int inflateResult;
	int toRead;
	int min_buffer_size = rw * rh * (REALBPP / 8) * 2;

	/* First make sure we have a large enough raw buffer to hold the
	 * decompressed data.  In practice, with a fixed REALBPP, fixed frame
	 * buffer size and the first update containing the entire frame
	 * buffer, this buffer allocation should only happen once, on the
	 * first update.
	 */
	if ( client->raw_buffer_size < min_buffer_size) {

		if ( client->raw_buffer != NULL ) {

			free( client->raw_buffer );

		}

		client->raw_buffer_size = min_buffer_size;
		client->raw_buffer = (char*) malloc( client->raw_buffer_size );

	}

	if (!ReadFromRFBServer(client, (char *)&header, sz_rfbZRLEHeader))
		return FALSE;

	remaining = rfbClientSwap32IfLE(header.length);

	/* Need to initialize the decompressor state. */
	client->decompStream.next_in   = ( Bytef * )client->buffer;
	client->decompStream.avail_in  = 0;
	client->decompStream.next_out  = ( Bytef * )client->raw_buffer;
	client->decompStream.avail_out = client->raw_buffer_size;
	client->decompStream.data_type = Z_BINARY;

	/* Initialize the decompression stream structures on the first invocation. */
	if ( client->decompStreamInited == FALSE ) {

		inflateResult = inflateInit( &client->decompStream );

		if ( inflateResult != Z_OK ) {
			rfbClientLog(
					"inflateInit returned error: %d, msg: %s\n",
					inflateResult,
					client->decompStream.msg);
			return FALSE;
		}

		client->decompStreamInited = TRUE;

	}

	inflateResult = Z_OK;

	/* Process buffer full of data until no more to process, or
	 * some type of inflater error, or Z_STREAM_END.
	 */
	while (( remaining > 0 ) &&
			( inflateResult == Z_OK )) {

		if ( remaining > RFB_BUFFER_SIZE ) {
			toRead = RFB_BUFFER_SIZE;
		}
		else {
			toRead = remaining;
		}

		/* Fill the buffer, obtaining data from the server. */
		if (!ReadFromRFBServer(client, client->buffer,toRead))
			return FALSE;

		client->decompStream.next_in  = ( Bytef * )client->buffer;
		client->decompStream.avail_in = toRead;

		/* Need to uncompress buffer full. */
		inflateResult = inflate( &client->decompStream, Z_SYNC_FLUSH );

		/* We never supply a dictionary for compression. */
		if ( inflateResult == Z_NEED_DICT ) {
			rfbClientLog("zlib inflate needs a dictionary!\n");
			return FALSE;
		}
		if ( inflateResult < 0 ) {
			rfbClientLog(
					"zlib inflate returned error: %d, msg: %s\n",
					inflateResult,
					client->decompStream.msg);
			return FALSE;
		}

		/* Result buffer allocated to be at least large enough.  We should
		 * never run out of space!
		 */
		if (( client->decompStream.avail_in > 0 ) &&
				( client->decompStream.avail_out <= 0 )) {
			rfbClientLog("zlib inflate ran out of space!\n");
			return FALSE;
		}

		remaining -= toRead;

	} /* while ( remaining > 0 ) */

	if ( inflateResult == Z_OK ) {
		void* buf=client->raw_buffer;
		int i,j;

		remaining = client->raw_buffer_size-client->decompStream.avail_out;

		for(j=0; j<rh; j+=rfbZRLETileHeight)
			for(i=0; i<rw; i+=rfbZRLETileWidth) {
				int subWidth=(i+rfbZRLETileWidth>rw)?rw-i:rfbZRLETileWidth;
				int subHeight=(j+rfbZRLETileHeight>rh)?rh-j:rfbZRLETileHeight;
				int result=HandleZRLETile(client,buf,remaining,rx+i,ry+j,subWidth,subHeight);

				if(result<0) {
					rfbClientLog("ZRLE decoding failed (%d)\n",result);
return TRUE;
					return FALSE;
				}

				buf+=result;
				remaining-=result;
			}
	}
	else {

		rfbClientLog(
				"zlib inflate returned error: %d, msg: %s\n",
				inflateResult,
				client->decompStream.msg);
		return FALSE;

	}

	return TRUE;
}

#if REALBPP!=BPP && defined(UNCOMP) && UNCOMP!=0
#if UNCOMP>0
#define UncompressCPixel(pointer) ((*(CARDBPP*)pointer)>>UNCOMP)
#else
#define UncompressCPixel(pointer) ((*(CARDBPP*)pointer)<<(-(UNCOMP)))
#endif
#else
#define UncompressCPixel(pointer) (*(CARDBPP*)pointer)
#endif

static int HandleZRLETile(rfbClient* client,
		uint8_t* buffer,size_t buffer_length,
		int x,int y,int w,int h) {
	uint8_t* buffer_copy = buffer;
	uint8_t* buffer_end = buffer+buffer_length;
	uint8_t type;

	if(buffer_length<1)
		return -2;

	type = *buffer;
	buffer++;
	switch(type) {
		case 0: /* raw */
		{
#if REALBPP!=BPP
			int i,j;

			if(1+w*h*REALBPP/8>buffer_length) {
				rfbClientLog("expected %d bytes, got only %d (%dx%d)\n",1+w*h*REALBPP/8,buffer_length,w,h);
				return -3;
			}

			for(j=y*client->width; j<(y+h)*client->width; j+=client->width)
				for(i=x; i<x+w; i++,buffer+=REALBPP/8)
					((CARDBPP*)client->frameBuffer)[j+i] = UncompressCPixel(buffer);
#else
			CopyRectangle(client, buffer, x, y, w, h);
			buffer+=w*h*REALBPP/8;
#endif
			break;
		}
		case 1: /* solid */
		{
			CARDBPP color = UncompressCPixel(buffer);

			if(1+REALBPP/8>buffer_length)
				return -4;
				
			FillRectangle(client, x, y, w, h, color);

			buffer+=REALBPP/8;

			break;
		}
		case 2 ... 127: /* packed Palette */
		{
			CARDBPP palette[16];
			int i,j,shift,
				bpp=(type>4?(type>16?8:4):(type>2?2:1)),
				mask=(1<<bpp)-1,
				divider=(8/bpp);

			if(1+type*REALBPP/8+((w+divider-1)/divider)*h>buffer_length)
				return -5;

			/* read palette */
			for(i=0; i<type; i++,buffer+=REALBPP/8)
				palette[i] = UncompressCPixel(buffer);

			/* read palettized pixels */
			for(j=y*client->width; j<(y+h)*client->width; j+=client->width) {
				for(i=x,shift=8-bpp; i<x+w; i++) {
					((CARDBPP*)client->frameBuffer)[j+i] = palette[((*buffer)>>shift)&mask];
					shift-=bpp;
					if(shift<0) {
						shift=8-bpp;
						buffer++;
					}
				}
				if(shift<8-bpp)
					buffer++;
			}

			break;
		}
		/* case 17 ... 127: not used, but valid */
		case 128: /* plain RLE */
		{
			int i=0,j=0;
			while(j<h) {
				int color,length;
				/* read color */
				if(buffer+REALBPP/8+1>buffer_end)
					return -7;
				color = UncompressCPixel(buffer);
				buffer+=REALBPP/8;
				/* read run length */
				length=1;
				while(*buffer==0xff) {
					if(buffer+1>=buffer_end)
						return -8;
					length+=*buffer;
					buffer++;
				}
				length+=*buffer;
				buffer++;
				while(j<h && length>0) {
					((CARDBPP*)client->frameBuffer)[(y+j)*client->width+x+i] = color;
					length--;
					i++;
					if(i>=w) {
						i=0;
						j++;
					}
				}
				if(length>0)
					rfbClientLog("Warning: possible ZRLE corruption\n");
			}

			break;
		}
		case 129: /* unused */
		{
			return -8;
		}
		case 130 ... 255: /* palette RLE */
		{
			CARDBPP palette[128];
			int i,j;

			if(2+(type-128)*REALBPP/8>buffer_length)
				return -9;

			/* read palette */
			for(i=0; i<type-128; i++,buffer+=REALBPP/8)
				palette[i] = UncompressCPixel(buffer);
			/* read palettized pixels */
			i=j=0;
			while(j<h) {
				int color,length;
				/* read color */
				if(buffer>=buffer_end)
					return -10;
				color = palette[(*buffer)&0x7f];
				length=1;
				if(*buffer&0x80) {
					if(buffer+1>=buffer_end)
						return -11;
					buffer++;
					/* read run length */
					while(*buffer==0xff) {
						if(buffer+1>=buffer_end)
							return -8;
						length+=*buffer;
						buffer++;
					}
					length+=*buffer;
				}
				buffer++;
				while(j<h && length>0) {
					((CARDBPP*)client->frameBuffer)[(y+j)*client->width+x+i] = color;
					length--;
					i++;
					if(i>=w) {
						i=0;
						j++;
					}
				}
				if(length>0)
					rfbClientLog("Warning: possible ZRLE corruption\n");
			}

			break;
		}
	}

	return buffer-buffer_copy;	
}

#undef CARDBPP
#undef CARDREALBPP
#undef HandleZRLE
#undef HandleZRLETile
#undef UncompressCPixel
#undef REALBPP

#endif

#undef UNCOMP
