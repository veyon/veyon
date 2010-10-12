/*
 * FastQImage.cpp - class FastQImage providing fast inline-QImage-manips
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License aint with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "FastQImage.h"

// the following code has been taken from pygame-library and and modified
// by Tobias Doerffel, 2008

/*
  pygame - Python Game Library
  Copyright (C) 2000-2001  Pete Shinners
  Copyright (C) 2007  Rene Dudfield, Richard Goedeken

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Pete Shinners
  pete@shinners.org
*/


#define ALIGN_SIZE 16

void aligned_free( void * _buf )
{
	if( _buf != NULL )
	{
		int *ptr2=(int *)_buf - 1;
		_buf = (char *)_buf- *ptr2;
		free(_buf);
	}
}




void * aligned_malloc( uint32_t _bytes )
{
	char *ptr,*ptr2,*aligned_ptr;
	int align_mask = ALIGN_SIZE- 1;
	ptr=(char *)malloc(_bytes +ALIGN_SIZE+ sizeof(int));
	if(ptr==NULL) return NULL;

	ptr2 = ptr + sizeof(int);
	aligned_ptr = ptr2 + (ALIGN_SIZE- ((size_t)ptr2 & align_mask));


	ptr2 = aligned_ptr - sizeof(int);
	*((int *)ptr2)=(int)(aligned_ptr - ptr);

	return aligned_ptr;
}



/* this function implements an area-averaging shrinking filter in the X-dimension */
static void filter_shrink_X_C(uint8_t *srcpix, uint8_t *dstpix, unsigned int height, unsigned int srcpitch, unsigned int dstpitch, unsigned int srcwidth, unsigned int dstwidth)
{
    const unsigned int srcdiff = srcpitch - (srcwidth * 4);
    const unsigned int dstdiff = dstpitch - (dstwidth * 4);

    const unsigned int xspace = 0x10000 * srcwidth / dstwidth; /* must be > 1 */
    const unsigned int xrecip = (int) ((long long) 0x100000000LL / xspace);
    for (unsigned int y = 0; y < height; y++)
    {
        uint32_t accumulate[4] = {0,0,0,0};
        unsigned int xcounter = xspace;
        for (unsigned int x = 0; x < srcwidth; x++)
        {
            if (xcounter > 0x10000)
            {
                accumulate[0] += (uint32_t) *srcpix++;
                accumulate[1] += (uint32_t) *srcpix++;
                accumulate[2] += (uint32_t) *srcpix++;
                accumulate[3] += (uint32_t) *srcpix++;
                xcounter -= 0x10000;
            }
            else
            {
                /* write out a destination pixel */
                *dstpix++ = (uint8_t) (((accumulate[0] + ((srcpix[0] * xcounter) >> 16)) * xrecip) >> 16);
                *dstpix++ = (uint8_t) (((accumulate[1] + ((srcpix[1] * xcounter) >> 16)) * xrecip) >> 16);
                *dstpix++ = (uint8_t) (((accumulate[2] + ((srcpix[2] * xcounter) >> 16)) * xrecip) >> 16);
                *dstpix++ = (uint8_t) (((accumulate[3] + ((srcpix[3] * xcounter) >> 16)) * xrecip) >> 16);
                /* reload the accumulator with the remainder of this pixel */
                const unsigned int xfrac = 0x10000 - xcounter;
                accumulate[0] = (uint32_t) ((*srcpix++ * xfrac) >> 16);
                accumulate[1] = (uint32_t) ((*srcpix++ * xfrac) >> 16);
                accumulate[2] = (uint32_t) ((*srcpix++ * xfrac) >> 16);
                accumulate[3] = (uint32_t) ((*srcpix++ * xfrac) >> 16);
                xcounter = xspace - xfrac;
            }
        }
        srcpix += srcdiff;
        dstpix += dstdiff;
    }
}

/* this function implements an area-averaging shrinking filter in the Y-dimension */
static void filter_shrink_Y_C(uint8_t *srcpix, uint8_t *dstpix, unsigned int width, unsigned int srcpitch, unsigned int dstpitch, unsigned int srcheight, unsigned int dstheight)
{
    uint16_t *templine;
    const unsigned int srcdiff = srcpitch - (width * 4);
    const unsigned int dstdiff = dstpitch - (width * 4);

    /* allocate and clear a memory area for storing the accumulator line */
    templine = (uint16_t *) aligned_malloc(dstpitch * 2);
    if (templine == NULL) return;
    memset(templine, 0, dstpitch * 2);

    const unsigned int yspace = 0x10000 * srcheight / dstheight; /* must be > 1 */
    const unsigned int yrecip = (unsigned int) ((long long) 0x100000000LL / yspace);
    unsigned int ycounter = yspace;
    for (unsigned int y = 0; y < srcheight; y++)
    {
        uint16_t *accumulate = templine;
        if (ycounter > 0x10000)
        {
            for (unsigned int x = 0; x < width; x++)
            {
                *accumulate++ += (uint16_t) *srcpix++;
                *accumulate++ += (uint16_t) *srcpix++;
                *accumulate++ += (uint16_t) *srcpix++;
                *accumulate++ += (uint16_t) *srcpix++;
            }
            ycounter -= 0x10000;
        }
        else
        {
            /* write out a destination line */
            for (unsigned int x = 0; x < width; x++)
            {
                *dstpix++ = (uint8_t) (((*accumulate++ + ((*srcpix++ * ycounter) >> 16)) * yrecip) >> 16);
                *dstpix++ = (uint8_t) (((*accumulate++ + ((*srcpix++ * ycounter) >> 16)) * yrecip) >> 16);
                *dstpix++ = (uint8_t) (((*accumulate++ + ((*srcpix++ * ycounter) >> 16)) * yrecip) >> 16);
                *dstpix++ = (uint8_t) (((*accumulate++ + ((*srcpix++ * ycounter) >> 16)) * yrecip) >> 16);
            }
            dstpix += dstdiff;
            /* reload the accumulator with the remainder of this line */
            accumulate = templine;
            srcpix -= 4 * width;
            const unsigned int yfrac = 0x10000 - ycounter;
            for (unsigned int x = 0; x < width; x++)
            {
                *accumulate++ = (uint16_t) ((*srcpix++ * yfrac) >> 16);
                *accumulate++ = (uint16_t) ((*srcpix++ * yfrac) >> 16);
                *accumulate++ = (uint16_t) ((*srcpix++ * yfrac) >> 16);
                *accumulate++ = (uint16_t) ((*srcpix++ * yfrac) >> 16);
            }
            ycounter = yspace - yfrac;
        }
        srcpix += srcdiff;
    } /* for (int y = 0; y < srcheight; y++) */

    /* free the temporary memory */
    aligned_free(templine);
}


/* this function implements a bilinear filter in the X-dimension */
static void filter_expand_X_C(uint8_t *srcpix, uint8_t *dstpix, unsigned int height, unsigned int srcpitch, unsigned int dstpitch, unsigned int srcwidth, unsigned int dstwidth)
{
    int dstdiff = dstpitch - (dstwidth * 4);
    int *xidx0, *xmult0, *xmult1;
    unsigned int x, y;

    /* Allocate memory for factors */
    xidx0 = (int*)aligned_malloc(dstwidth * 4);
    if (xidx0 == NULL) return;
    xmult0 = (int *) aligned_malloc(dstwidth * 4);
    xmult1 = (int *) aligned_malloc(dstwidth * 4);
    if (xmult0 == NULL || xmult1 == NULL)
    {
        aligned_free(xidx0);
        if (xmult0) aligned_free(xmult0);
        if (xmult1) aligned_free(xmult1);
    }

    /* Create multiplier factors and starting indices and put them in arrays */
    for (x = 0; x < dstwidth; x++)
    {
        xidx0[x] = x * (srcwidth - 1) / dstwidth;
        xmult1[x] = 0x10000 * ((x * (srcwidth - 1)) % dstwidth) / dstwidth;
        xmult0[x] = 0x10000 - xmult1[x];
    }

    /* Do the scaling in raster order so we don't trash the cache */
    for (y = 0; y < height; y++)
    {
        uint8_t *srcrow0 = srcpix + y * srcpitch;
        for (x = 0; x < dstwidth; x++)
        {
            uint8_t *src = srcrow0 + xidx0[x] * 4;
            int xm0 = xmult0[x];
            int xm1 = xmult1[x];
            *dstpix++ = (uint8_t) (((src[0] * xm0) + (src[4] * xm1)) >> 16);
            *dstpix++ = (uint8_t) (((src[1] * xm0) + (src[5] * xm1)) >> 16);
            *dstpix++ = (uint8_t) (((src[2] * xm0) + (src[6] * xm1)) >> 16);
            *dstpix++ = (uint8_t) (((src[3] * xm0) + (src[7] * xm1)) >> 16);
        }
        dstpix += dstdiff;
    }

    /* free memory */
    aligned_free(xidx0);
    aligned_free(xmult0);
    aligned_free(xmult1);
}

/* this function implements a bilinear filter in the Y-dimension */
static void filter_expand_Y_C(uint8_t *srcpix, uint8_t *dstpix, unsigned int width, int unsigned srcpitch, unsigned int/* dstpitch*/, unsigned int srcheight, unsigned int dstheight)
{
    for (unsigned int y = 0; y < dstheight; y++)
    {
        const unsigned int yidx0 = y * (srcheight - 1) / dstheight;
        uint8_t *srcrow0 = srcpix + yidx0 * srcpitch;
        uint8_t *srcrow1 = srcrow0 + srcpitch;
        unsigned int ymult1 = 0x10000 * ((y * (srcheight - 1)) % dstheight) / dstheight;
        unsigned int ymult0 = 0x10000 - ymult1;
        for (unsigned int x = 0; x < width; x++)
        {
            *dstpix++ = (uint8_t) (((*srcrow0++ * ymult0) + (*srcrow1++ * ymult1)) >> 16);
            *dstpix++ = (uint8_t) (((*srcrow0++ * ymult0) + (*srcrow1++ * ymult1)) >> 16);
            *dstpix++ = (uint8_t) (((*srcrow0++ * ymult0) + (*srcrow1++ * ymult1)) >> 16);
            *dstpix++ = (uint8_t) (((*srcrow0++ * ymult0) + (*srcrow1++ * ymult1)) >> 16);
        }
    }
}


#if defined(__GNUC__) && defined(__i386__) /* || defined(__x86_64__))*/
#define USE_MMX
#endif

#ifdef USE_MMX

/* this function implements an area-averaging shrinking filter in the X-dimension */
static void filter_shrink_X_MMX(uint8_t *srcpix, uint8_t *dstpix, unsigned int height, unsigned int srcpitch, unsigned int dstpitch, unsigned int srcwidth, unsigned int dstwidth)
{
    const unsigned int srcdiff = srcpitch - (srcwidth * 4);
    const unsigned int dstdiff = dstpitch - (dstwidth * 4);

#if defined(__x86_64__)
    const unsigned int xspace = 0x04000 * srcwidth / dstwidth; /* must be > 1 */
    const unsigned int xrecip = (int) ((long long) 0x040000000 / xspace);
    long long srcdiff64 = srcdiff;
    long long dstdiff64 = dstdiff;
    long long One64 = 0x4000400040004000LL;
    asm(" /* MMX code for X-shrink area average filter */ "
        " pxor          %%mm0,      %%mm0;           "
        " movd             %6,      %%mm7;           " /* mm7 == xrecipmmx */
        " movq             %2,      %%mm6;           " /* mm6 = 2^14  */
        " pshufw    $0, %%mm7,      %%mm7;           "
        "1:                                          " /* outer Y-loop */
        " movl             %5,      %%ecx;           " /* ecx == xcounter */
        " pxor          %%mm1,      %%mm1;           " /* mm1 == accumulator */
        " movl             %4,      %%edx;           " /* edx == width */
        "2:                                          " /* inner X-loop */
        " cmpl        $0x4000,      %%ecx;           "
        " jbe              3f;                       "
        " movd           (%0),      %%mm2;           " /* mm2 = srcpix */
        " add              $4,         %0;           "
        " punpcklbw     %%mm0,      %%mm2;           "
        " paddw         %%mm2,      %%mm1;           " /* accumulator += srcpix */
        " subl        $0x4000,      %%ecx;           "
        " jmp              4f;                       "
        "3:                                          " /* prepare to output a pixel */
        " movd          %%ecx,      %%mm2;           "
        " movq          %%mm6,      %%mm3;           " /* mm3 = 2^14  */
        " pshufw    $0, %%mm2,      %%mm2;           "
        " movd           (%0),      %%mm4;           " /* mm4 = srcpix */
        " add              $4,         %0;           "
        " punpcklbw     %%mm0,      %%mm4;           "
        " psubw         %%mm2,      %%mm3;           " /* mm3 = xfrac */
        " psllw            $2,      %%mm4;           "
        " pmulhuw       %%mm4,      %%mm2;           " /* mm2 = (srcpix * xcounter >> 16) */
        " pmulhuw       %%mm4,      %%mm3;           " /* mm3 = (srcpix * xfrac) >> 16 */
        " paddw         %%mm1,      %%mm2;           "
        " movq          %%mm3,      %%mm1;           " /* accumulator = (srcpix * xfrac) >> 16 */
        " pmulhuw       %%mm7,      %%mm2;           "
        " packuswb      %%mm0,      %%mm2;           "
        " movd          %%mm2,       (%1);           "
        " add              %5,      %%ecx;           "
        " add              $4,         %1;           "
        " subl        $0x4000,      %%ecx;           "
        "4:                                          " /* tail of inner X-loop */
        " decl          %%edx;                       "
        " jne              2b;                       "
        " add              %7,         %0;           " /* srcpix += srcdiff */
        " add              %8,         %1;           " /* dstpix += dstdiff */
        " decl             %3;                       "
        " jne              1b;                       "
        " emms;                                      "
        :                   /* no outputs */
        : "r"(srcpix), "r"(dstpix), "m"(One64),     "m"(height),   "m"(srcwidth),
          "m"(xspace), "m"(xrecip), "m"(srcdiff64), "m"(dstdiff64)                /* input */
        : "%ecx","%edx"     /* clobbered */
        );
#elif defined(__i386__)
    const unsigned int xspace = 0x04000 * srcwidth / dstwidth; /* must be > 1 */
    const unsigned int xrecip = (int) ((long long) 0x040000000 / xspace);
    long long One64 = 0x4000400040004000LL;

    asm(" /* MMX code for X-shrink area average filter */ "
        " pxor          %%mm0,      %%mm0;           "
        " movd             %6,      %%mm7;           " /* mm7 == xrecipmmx */
        " movq             %2,      %%mm6;           " /* mm6 = 2^14  */
        " pshufw    $0, %%mm7,      %%mm7;           "
        "1:                                          " /* outer Y-loop */
        " movl             %5,      %%ecx;           " /* ecx == xcounter */
        " pxor          %%mm1,      %%mm1;           " /* mm1 == accumulator */
        " movl             %4,      %%edx;           " /* edx == width */
        "2:                                          " /* inner X-loop */
        " cmpl        $0x4000,      %%ecx;           "
        " jbe              3f;                       "
        " movd           (%0),      %%mm2;           " /* mm2 = srcpix */
        " add              $4,         %0;           "
        " punpcklbw     %%mm0,      %%mm2;           "
        " paddw         %%mm2,      %%mm1;           " /* accumulator += srcpix */
        " subl        $0x4000,      %%ecx;           "
        " jmp              4f;                       "
        "3:                                          " /* prepare to output a pixel */
        " movd          %%ecx,      %%mm2;           "
        " movq          %%mm6,      %%mm3;           " /* mm3 = 2^14  */
        " pshufw    $0, %%mm2,      %%mm2;           "
        " movd           (%0),      %%mm4;           " /* mm4 = srcpix */
        " add              $4,         %0;           "
        " punpcklbw     %%mm0,      %%mm4;           "
        " psubw         %%mm2,      %%mm3;           " /* mm3 = xfrac */
        " psllw            $2,      %%mm4;           "
        " pmulhuw       %%mm4,      %%mm2;           " /* mm2 = (srcpix * xcounter >> 16) */
        " pmulhuw       %%mm4,      %%mm3;           " /* mm3 = (srcpix * xfrac) >> 16 */
        " paddw         %%mm1,      %%mm2;           "
        " movq          %%mm3,      %%mm1;           " /* accumulator = (srcpix * xfrac) >> 16 */
        " pmulhuw       %%mm7,      %%mm2;           "
        " packuswb      %%mm0,      %%mm2;           "
        " movd          %%mm2,       (%1);           "
        " add              %5,      %%ecx;           "
        " add              $4,         %1;           "
        " subl        $0x4000,      %%ecx;           "
        "4:                                          " /* tail of inner X-loop */
        " decl          %%edx;                       "
        " jne              2b;                       "
        " add              %7,         %0;           " /* srcpix += srcdiff */
        " add              %8,         %1;           " /* dstpix += dstdiff */
        " decl             %3;                       "
        " jne              1b;                       "
        :                   /* no outputs */
        : "S"(srcpix), "D"(dstpix), "m"(One64),     "m"(height),   "m"(srcwidth),
          "m"(xspace), "m"(xrecip), "m"(srcdiff),   "m"(dstdiff)                  /* input */
        : "%ecx","%edx"     /* clobbered */
        );
        asm(" emms;                                      "
            :
            :
            : "%esi", "%edi");
#endif
}

/* this function implements an area-averaging shrinking filter in the Y-dimension */
static void filter_shrink_Y_MMX(uint8_t *srcpix, uint8_t *dstpix, unsigned int width, unsigned int srcpitch, unsigned int dstpitch, unsigned int srcheight, unsigned int dstheight)
{
    uint16_t *templine;
    int srcdiff = srcpitch - (width * 4);
    int dstdiff = dstpitch - (width * 4);

    /* allocate and clear a memory area for storing the accumulator line */
    templine = (uint16_t *) aligned_malloc(dstpitch * 2);
    if (templine == NULL) return;
    memset(templine, 0, dstpitch * 2);

#if defined(__x86_64__)
    int yspace = 0x4000 * srcheight / dstheight; /* must be > 1 */
    int yrecip = (int) ((long long) 0x040000000 / yspace);
    long long srcdiff64 = srcdiff;
    long long dstdiff64 = dstdiff;
    long long One64 = 0x4000400040004000LL;
    asm(" /* MMX code for Y-shrink area average filter */ "
        " movl             %5,      %%ecx;           " /* ecx == ycounter */
        " pxor          %%mm0,      %%mm0;           "
        " movd             %6,      %%mm7;           " /* mm7 == yrecipmmx */
        " pshufw    $0, %%mm7,      %%mm7;           "
        "1:                                          " /* outer Y-loop */
        " mov              %2,      %%rax;           " /* rax == accumulate */
        " cmpl        $0x4000,      %%ecx;           "
        " jbe              3f;                       "
        " movl             %4,      %%edx;           " /* edx == width */
        "2:                                          "
        " movd           (%0),      %%mm1;           "
        " add              $4,         %0;           "
        " movq        (%%rax),      %%mm2;           "
        " punpcklbw     %%mm0,      %%mm1;           "
        " paddw         %%mm1,      %%mm2;           "
        " movq          %%mm2,    (%%rax);           "
        " add              $8,      %%rax;           "
        " decl          %%edx;                       "
        " jne              2b;                       "
        " subl        $0x4000,      %%ecx;           "
        " jmp              6f;                       "
        "3:                                          " /* prepare to output a line */
        " movd          %%ecx,      %%mm1;           "
        " movl             %4,      %%edx;           " /* edx = width */
        " movq             %9,      %%mm6;           " /* mm6 = 2^14  */
        " pshufw    $0, %%mm1,      %%mm1;           "
        " psubw         %%mm1,      %%mm6;           " /* mm6 = yfrac */
        "4:                                          "
        " movd           (%0),      %%mm4;           " /* mm4 = srcpix */
        " add              $4,         %0;           "
        " punpcklbw     %%mm0,      %%mm4;           "
        " movq        (%%rax),      %%mm5;           " /* mm5 = accumulate */
        " movq          %%mm6,      %%mm3;           "
        " psllw            $2,      %%mm4;           "
        " pmulhuw       %%mm4,      %%mm3;           " /* mm3 = (srcpix * yfrac) >> 16 */
        " pmulhuw       %%mm1,      %%mm4;           " /* mm4 = (srcpix * ycounter >> 16) */
        " movq          %%mm3,    (%%rax);           "
        " paddw         %%mm5,      %%mm4;           "
        " add              $8,      %%rax;           "
        " pmulhuw       %%mm7,      %%mm4;           "
        " packuswb      %%mm0,      %%mm4;           "
        " movd          %%mm4,       (%1);           "
        " add              $4,         %1;           "
        " decl          %%edx;                       "
        " jne              4b;                       "
        " add              %8,         %1;           " /* dstpix += dstdiff */
        " addl             %5,      %%ecx;           "
        " subl        $0x4000,      %%ecx;           "
        "6:                                          " /* tail of outer Y-loop */
        " add              %7,         %0;           " /* srcpix += srcdiff */
        " decl             %3;                       "
        " jne              1b;                       "
        " emms;                                      "
        :                   /* no outputs */
        : "r"(srcpix), "r"(dstpix), "m"(templine),  "m"(srcheight), "m"(width),
          "m"(yspace), "m"(yrecip), "m"(srcdiff64), "m"(dstdiff64), "m"(One64)  /* input */
        : "%ecx","%edx","%rax"     /* clobbered */
        );
#elif defined(__i386__)
    int yspace = 0x4000 * srcheight / dstheight; /* must be > 1 */
    int yrecip = (int) ((long long) 0x040000000 / yspace);
    long long One64 = 0x4000400040004000LL;
    asm(" /* MMX code for Y-shrink area average filter */ "
        " movl             %5,      %%ecx;           " /* ecx == ycounter */
        " pxor          %%mm0,      %%mm0;           "
        " movd             %6,      %%mm7;           " /* mm7 == yrecipmmx */
        " pshufw    $0, %%mm7,      %%mm7;           "
        "1:                                          " /* outer Y-loop */
        " movl             %2,      %%eax;           " /* rax == accumulate */
        " cmpl        $0x4000,      %%ecx;           "
        " jbe              3f;                       "
        " movl             %4,      %%edx;           " /* edx == width */
        "2:                                          "
        " movd           (%0),      %%mm1;           "
        " add              $4,         %0;           "
        " movq        (%%eax),      %%mm2;           "
        " punpcklbw     %%mm0,      %%mm1;           "
        " paddw         %%mm1,      %%mm2;           "
        " movq          %%mm2,    (%%eax);           "
        " add              $8,      %%eax;           "
        " decl          %%edx;                       "
        " jne              2b;                       "
        " subl        $0x4000,      %%ecx;           "
        " jmp              6f;                       "
        "3:                                          " /* prepare to output a line */
        " movd          %%ecx,      %%mm1;           "
        " movl             %4,      %%edx;           " /* edx = width */
        " movq             %9,      %%mm6;           " /* mm6 = 2^14  */
        " pshufw    $0, %%mm1,      %%mm1;           "
        " psubw         %%mm1,      %%mm6;           " /* mm6 = yfrac */
        "4:                                          "
        " movd           (%0),      %%mm4;           " /* mm4 = srcpix */
        " add              $4,         %0;           "
        " punpcklbw     %%mm0,      %%mm4;           "
        " movq        (%%eax),      %%mm5;           " /* mm5 = accumulate */
        " movq          %%mm6,      %%mm3;           "
        " psllw            $2,      %%mm4;           "
        " pmulhuw       %%mm4,      %%mm3;           " /* mm3 = (srcpix * yfrac) >> 16 */
        " pmulhuw       %%mm1,      %%mm4;           " /* mm4 = (srcpix * ycounter >> 16) */
        " movq          %%mm3,    (%%eax);           "
        " paddw         %%mm5,      %%mm4;           "
        " add              $8,      %%eax;           "
        " pmulhuw       %%mm7,      %%mm4;           "
        " packuswb      %%mm0,      %%mm4;           "
        " movd          %%mm4,       (%1);           "
        " add              $4,         %1;           "
        " decl          %%edx;                       "
        " jne              4b;                       "
        " add              %8,         %1;           " /* dstpix += dstdiff */
        " addl             %5,      %%ecx;           "
        " subl        $0x4000,      %%ecx;           "
        "6:                                          " /* tail of outer Y-loop */
        " add              %7,         %0;           " /* srcpix += srcdiff */
        " decl             %3;                       "
        " jne              1b;                       "
        " emms;                                      "
        :                   /* no outputs */
        : "S"(srcpix), "D"(dstpix), "m"(templine),  "m"(srcheight), "m"(width),
          "m"(yspace), "m"(yrecip), "m"(srcdiff),   "m"(dstdiff),   "m"(One64)  /* input */
        : "%ecx","%edx","%rax"     /* clobbered */
        );
        asm(" emms;                                      "
            :
            :
            : "%esi", "%edi");
#endif
    /* free the temporary memory */
    aligned_free(templine);
}


/* this function implements a bilinear filter in the X-dimension */
static void filter_expand_X_MMX(uint8_t *srcpix, uint8_t *dstpix, unsigned int height, unsigned int srcpitch, unsigned int dstpitch, unsigned int srcwidth, unsigned int dstwidth)
{
    int *xidx0, *xmult0, *xmult1;
    unsigned int x, y;

    /* Allocate memory for factors */
    xidx0 = (int*)aligned_malloc(dstwidth * 4);
    if (xidx0 == NULL) return;
    xmult0 = (int *) aligned_malloc(dstwidth * 8);
    xmult1 = (int *) aligned_malloc(dstwidth * 8);
    if (xmult0 == NULL || xmult1 == NULL)
    {
        aligned_free(xidx0);
        if (xmult0) aligned_free(xmult0);
        if (xmult1) aligned_free(xmult1);
    }

    /* Create multiplier factors and starting indices and put them in arrays */
    for (x = 0; x < dstwidth; x++)
    {
        xidx0[x] = x * (srcwidth - 1) / dstwidth;
        const unsigned int xm1 = 0x100 * ((x * (srcwidth - 1)) % dstwidth) / dstwidth;
        const unsigned int xm0 = 0x100 - xm1;
        xmult1[x*2]   = xm1 | (xm1 << 16);
        xmult1[x*2+1] = xm1 | (xm1 << 16);
        xmult0[x*2]   = xm0 | (xm0 << 16);
        xmult0[x*2+1] = xm0 | (xm0 << 16);
    }

    /* Do the scaling in raster order so we don't trash the cache */
    for (y = 0; y < height; y++)
    {
        uint8_t *srcrow0 = srcpix + y * srcpitch;
        uint8_t *dstrow = dstpix + y * dstpitch;
        int *xm0 = xmult0;
        int *x0 = xidx0;
#if defined(__x86_64__)
        int *xm1 = xmult1;
        asm( " /* MMX code for inner loop of Y bilinear filter */ "
             " movl             %3,      %%ecx;           "
             " pxor          %%mm0,      %%mm0;           "
             "1:                                          "
             " movsxl         (%5),      %%rax;           " /* get xidx0[x] */
             " add              $4,         %5;           "
             " movq           (%1),      %%mm1;           " /* load mult0 */
             " add              $8,         %1;           "
             " movq           (%2),      %%mm2;           " /* load mult1 */
             " add              $8,         %2;           "
             " movd   (%0,%%rax,4),      %%mm4;           "
             " movd  4(%0,%%rax,4),      %%mm5;           "
             " punpcklbw     %%mm0,      %%mm4;           "
             " punpcklbw     %%mm0,      %%mm5;           "
             " pmullw        %%mm1,      %%mm4;           "
             " pmullw        %%mm2,      %%mm5;           "
             " paddw         %%mm4,      %%mm5;           "
             " psrlw            $8,      %%mm5;           "
             " packuswb      %%mm0,      %%mm5;           "
             " movd          %%mm5,       (%4);           "
             " add              $4,         %4;           "
             " decl          %%ecx;                       "
             " jne              1b;                       "
             " emms;                                      "
             :                   /* no outputs */
             : "r"(srcrow0), "r"(xm0), "r"(xm1), "m"(dstwidth), "r"(dstrow), "r"(x0)  /* input */
             : "%ecx","%rax"     /* clobbered */
             );
#elif defined(__i386__)
	int width = dstwidth;
	long long One64 = 0x0100010001000100LL;
        asm( " /* MMX code for inner loop of Y bilinear filter */ "
             " pxor          %%mm0,      %%mm0;           "
             " movq             %2,      %%mm7;           "
             "1:                                          "
             " movl           (%5),      %%eax;           " /* get xidx0[x] */
             " add              $4,         %5;           "
             " movq          %%mm7,      %%mm2;           "
             " movq           (%1),      %%mm1;           " /* load mult0 */
             " add              $8,         %1;           "
             " psubw         %%mm1,      %%mm2;           " /* load mult1 */
             " movd   (%0,%%eax,4),      %%mm4;           "
             " movd  4(%0,%%eax,4),      %%mm5;           "
             " punpcklbw     %%mm0,      %%mm4;           "
             " punpcklbw     %%mm0,      %%mm5;           "
             " pmullw        %%mm1,      %%mm4;           "
             " pmullw        %%mm2,      %%mm5;           "
             " paddw         %%mm4,      %%mm5;           "
             " psrlw            $8,      %%mm5;           "
             " packuswb      %%mm0,      %%mm5;           "
             " movd          %%mm5,       (%4);           "
             " add              $4,         %4;           "
             " decl             %3;                       "
             " jne              1b;                       "
             :                   /* no outputs */
             : "S"(srcrow0), "c"(xm0), "m"(One64), "m"(width), "D"(dstrow), "d"(x0)  /* input */
             : "%eax"     /* clobbered */
             );
        asm(" emms;                                      "
            :
            :
            : "%ecx", "%esi", "%edi", "%edx");
#endif
    }

    /* free memory */
    aligned_free(xidx0);
    aligned_free(xmult0);
    aligned_free(xmult1);
}

/* this function implements a bilinear filter in the Y-dimension */
static void filter_expand_Y_MMX(uint8_t *srcpix, uint8_t *dstpix, unsigned int width, unsigned int srcpitch, unsigned int dstpitch, unsigned int srcheight, unsigned int dstheight)
{
    for (unsigned int y = 0; y < dstheight; y++)
    {
        int yidx0 = y * (srcheight - 1) / dstheight;
        uint8_t *srcrow0 = srcpix + yidx0 * srcpitch;
        uint8_t *srcrow1 = srcrow0 + srcpitch;
        int ymult1 = 0x0100 * ((y * (srcheight - 1)) % dstheight) / dstheight;
        int ymult0 = 0x0100 - ymult1;
        uint8_t *dstrow = dstpix + y * dstpitch;
#if defined(__x86_64__)
        asm( " /* MMX code for inner loop of Y bilinear filter */ "
             " movl          %4,      %%ecx;                      "
             " movd          %2,      %%mm1;                      "
             " movd          %3,      %%mm2;                      "
             " pxor       %%mm0,      %%mm0;                      "
             " pshufw      $0, %%mm1, %%mm1;                      "
             " pshufw      $0, %%mm2, %%mm2;                      "
             "1:                                                  "
             " movd        (%0),      %%mm4;                      "
             " add           $4,         %0;                      "
             " movd        (%1),      %%mm5;                      "
             " add           $4,         %1;                      "
             " punpcklbw  %%mm0,     %%mm4;                       "
             " punpcklbw  %%mm0,     %%mm5;                       "
             " pmullw     %%mm1,     %%mm4;                       "
             " pmullw     %%mm2,     %%mm5;                       "
             " paddw      %%mm4,     %%mm5;                       "
             " psrlw         $8,     %%mm5;                       "
             " packuswb   %%mm0,     %%mm5;                       "
             " movd       %%mm5,      (%5);                       "
             " add           $4,        %5;                       "
             " decl       %%ecx;                                  "
             " jne           1b;                                  "
             " emms;                                              "
             :                /* no outputs */
             : "r"(srcrow0), "r"(srcrow1), "m"(ymult0), "m"(ymult1), "m"(width), "r"(dstrow)  /* input */
             : "%ecx"         /* clobbered */
             );
#elif defined(__i386__)
        asm( " /* MMX code for inner loop of Y bilinear filter */ "
             " movl          %4,      %%eax;                      "
             " movd          %2,      %%mm1;                      "
             " movd          %3,      %%mm2;                      "
             " pxor       %%mm0,      %%mm0;                      "
             " pshufw      $0, %%mm1, %%mm1;                      "
             " pshufw      $0, %%mm2, %%mm2;                      "
             "1:                                                  "
             " movd        (%0),      %%mm4;                      "
             " add           $4,         %0;                      "
             " movd        (%1),      %%mm5;                      "
             " add           $4,         %1;                      "
             " punpcklbw  %%mm0,     %%mm4;                       "
             " punpcklbw  %%mm0,     %%mm5;                       "
             " pmullw     %%mm1,     %%mm4;                       "
             " pmullw     %%mm2,     %%mm5;                       "
             " paddw      %%mm4,     %%mm5;                       "
             " psrlw         $8,     %%mm5;                       "
             " packuswb   %%mm0,     %%mm5;                       "
             " movd       %%mm5,      (%5);                       "
             " add           $4,        %5;                       "
             " decl       %%eax;                                  "
             " jne           1b;                                  "
             :                /* no outputs */
             : "S"(srcrow0), "c"(srcrow1), "m"(ymult0), "m"(ymult1), "m"(width), "D"(dstrow)  /* input */
             : "%eax"         /* clobbered */
             );
        asm(" emms;                                      "
            :
            :
            : "%esi", "%edi", "%ecx");
#endif
    }
}
#endif


typedef void (*filter_fn)(uint8_t*, uint8_t*, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);

static filter_fn filter_shrink_X = filter_shrink_X_C;
static filter_fn filter_shrink_Y = filter_shrink_Y_C;
static filter_fn filter_expand_X = filter_expand_X_C;
static filter_fn filter_expand_Y = filter_expand_Y_C;

QImage & FastQImage::scaleTo( QImage & _dst ) const
{
	if( size() == _dst.size() )
	{
		return _dst = *this;
	}
	if( !_dst.size().isValid() )
	{
		return _dst = QImage();
	}
	if( format() == Format_Invalid )
	{
		return _dst;
	}
	if( format() != Format_ARGB32 && format() != Format_RGB32 &&
				format() != Format_ARGB32_Premultiplied )
	{
		qWarning( "FastQImage::scaleTo(...): converting source-image "
							"to Format_ARGB32" );
		return FastQImage( convertToFormat( Format_ARGB32 ) ).
							scaleTo( _dst );
	}

#ifdef USE_MMX
#if !defined(__x86_64__)
	static bool MMX_checked = false;
	if( MMX_checked == false )
	{
		int features = 0;
		asm(	"pushl	%%ebx;		\n"
			"mov	$1,%%eax;	\n"
			"cpuid;			\n"
			"popl	%%ebx;		\n"
			: "=d"( features ) : : "%eax", "%ecx" );
		if( features & 0x00800000 )
		{
#endif
			filter_shrink_X = filter_shrink_X_MMX;
			filter_shrink_Y = filter_shrink_Y_MMX;
			filter_expand_X = filter_expand_X_MMX;
			filter_expand_Y = filter_expand_Y_MMX;
#if !defined(__x86_64__)
		}

		MMX_checked = true;
	}
#endif
#endif
	uint8_t* srcpix = (uint8_t*)bits();
	uint8_t* dstpix = (uint8_t*)_dst.bits();
	int srcpitch = width()*4;
	int dstpitch = _dst.width()*4;

	const int srcwidth = width();
	const int srcheight = height();
	const int dstwidth = _dst.width();
	const int dstheight = _dst.height();

	// Create a temporary processing buffer if we will be scaling both X and Y
	uint8_t *temppix = NULL;
	int tempwidth=0, temppitch=0, tempheight=0;
	if (srcwidth != dstwidth && srcheight != dstheight)
	{
		tempwidth = dstwidth;
		temppitch = tempwidth << 2;
		tempheight = srcheight;
		temppix = (uint8_t *) aligned_malloc(temppitch * tempheight);
		if (temppix == NULL)
		{
			return _dst;
		}
	}

	/* Start the filter by doing X-scaling */
	if (dstwidth < srcwidth) /* shrink */
	{
		if (srcheight != dstheight)
		{
		    filter_shrink_X(srcpix, temppix, srcheight, srcpitch, temppitch, srcwidth, dstwidth);
		}
		else
		{
		    filter_shrink_X(srcpix, dstpix, srcheight, srcpitch, dstpitch, srcwidth, dstwidth);
		}
	}
	else if (dstwidth > srcwidth) /* expand */
	{
		if (srcheight != dstheight)
		{
		    filter_expand_X(srcpix, temppix, srcheight, srcpitch, temppitch, srcwidth, dstwidth);
		}
		else
		{
		    filter_expand_X(srcpix, dstpix, srcheight, srcpitch, dstpitch, srcwidth, dstwidth);
		}
	}

	/* Now do the Y scale */
	if (dstheight < srcheight) /* shrink */
	{
		if (srcwidth != dstwidth)
		{
		    filter_shrink_Y(temppix, dstpix, tempwidth, temppitch, dstpitch, srcheight, dstheight);
		}
		else
		{
		    filter_shrink_Y(srcpix, dstpix, srcwidth, srcpitch, dstpitch, srcheight, dstheight);
		}
	}
	else if (dstheight > srcheight)  /* expand */
	{
		if (srcwidth != dstwidth)
		{
		    filter_expand_Y(temppix, dstpix, tempwidth, temppitch, dstpitch, srcheight, dstheight);
		}
		else
		{
		    filter_expand_Y(srcpix, dstpix, srcwidth, srcpitch, dstpitch, srcheight, dstheight);
		}
	}

	aligned_free( temppix );

	return _dst;
}


