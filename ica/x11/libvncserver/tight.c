/*
 * tight.c
 *
 * Routines to implement Tight Encoding
 *
 * Our Tight encoder is based roughly on the TurboVNC v0.6 encoder with some
 * additional enhancements from TurboVNC 1.1.  For lower compression levels,
 * this encoder provides a tremendous reduction in CPU usage (and subsequently,
 * an increase in throughput for CPU-limited environments) relative to the
 * TightVNC encoder, whereas Compression Level 9 provides a low-bandwidth mode
 * that behaves similarly to Compression Levels 5-9 in the old TightVNC
 * encoder.
 */

/*
 *  Copyright (C) 2010-2012 D. R. Commander.  All Rights Reserved.
 *  Copyright (C) 2005-2008 Sun Microsystems, Inc.  All Rights Reserved.
 *  Copyright (C) 2004 Landmark Graphics Corporation.  All Rights Reserved.
 *  Copyright (C) 2000, 2001 Const Kaplinsky.  All Rights Reserved.
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 *  USA.
 */

#include <rfb/rfb.h>
#include "private.h"

#ifdef LIBVNCSERVER_HAVE_LIBPNG
#include <png.h>
#endif
#include "turbojpeg.h"


/* Note: The following constant should not be changed. */
#define TIGHT_MIN_TO_COMPRESS 12

/* The parameters below may be adjusted. */
#define MIN_SPLIT_RECT_SIZE     4096
#define MIN_SOLID_SUBRECT_SIZE  2048
#define MAX_SPLIT_TILE_SIZE       16

/*
 * There is so much access of the Tight encoding static data buffers
 * that we resort to using thread local storage instead of having
 * per-client data.
 */
#if LIBVNCSERVER_HAVE_LIBPTHREAD && LIBVNCSERVER_HAVE_TLS && !defined(TLS) && defined(__linux__)
#define TLS __thread
#endif
#ifndef TLS
#define TLS
#endif

/* This variable is set on every rfbSendRectEncodingTight() call. */
static TLS rfbBool usePixelFormat24 = FALSE;


/* Compression level stuff. The following array contains various
   encoder parameters for each of 10 compression levels (0..9).
   Last three parameters correspond to JPEG quality levels (0..9). */

typedef struct TIGHT_CONF_s {
    int maxRectSize, maxRectWidth;
    int monoMinRectSize;
    int idxZlibLevel, monoZlibLevel, rawZlibLevel;
    int idxMaxColorsDivisor;
    int palMaxColorsWithJPEG;
} TIGHT_CONF;

static TIGHT_CONF tightConf[4] = {
    { 65536, 2048,   6, 0, 0, 0,   4, 24 }, /* 0  (used only without JPEG) */
    { 65536, 2048,  32, 1, 1, 1,  96, 24 }, /* 1 */
    { 65536, 2048,  32, 3, 3, 2,  96, 96 }, /* 2  (used only with JPEG) */
    { 65536, 2048,  32, 7, 7, 5,  96, 256 } /* 9 */
};

#ifdef LIBVNCSERVER_HAVE_LIBPNG
typedef struct TIGHT_PNG_CONF_s {
    int png_zlib_level, png_filters;
} TIGHT_PNG_CONF;

static TIGHT_PNG_CONF tightPngConf[10] = {
    { 0, PNG_NO_FILTERS },
    { 1, PNG_NO_FILTERS },
    { 2, PNG_NO_FILTERS },
    { 3, PNG_NO_FILTERS },
    { 4, PNG_NO_FILTERS },
    { 5, PNG_ALL_FILTERS },
    { 6, PNG_ALL_FILTERS },
    { 7, PNG_ALL_FILTERS },
    { 8, PNG_ALL_FILTERS },
    { 9, PNG_ALL_FILTERS },
};
#endif

static TLS int compressLevel = 1;
static TLS int qualityLevel = 95;
static TLS int subsampLevel = TJ_444;

static const int subsampLevel2tjsubsamp[4] = {
    TJ_444, TJ_420, TJ_422, TJ_GRAYSCALE
};


/* Stuff dealing with palettes. */

typedef struct COLOR_LIST_s {
    struct COLOR_LIST_s *next;
    int idx;
    uint32_t rgb;
} COLOR_LIST;

typedef struct PALETTE_ENTRY_s {
    COLOR_LIST *listNode;
    int numPixels;
} PALETTE_ENTRY;

typedef struct PALETTE_s {
    PALETTE_ENTRY entry[256];
    COLOR_LIST *hash[256];
    COLOR_LIST list[256];
} PALETTE;

/* TODO: move into rfbScreen struct */
static TLS int paletteNumColors = 0;
static TLS int paletteMaxColors = 0;
static TLS uint32_t monoBackground = 0;
static TLS uint32_t monoForeground = 0;
static TLS PALETTE palette;

/* Pointers to dynamically-allocated buffers. */

static TLS int tightBeforeBufSize = 0;
static TLS char *tightBeforeBuf = NULL;

static TLS int tightAfterBufSize = 0;
static TLS char *tightAfterBuf = NULL;

static TLS tjhandle j = NULL;

void rfbTightCleanup (rfbScreenInfoPtr screen)
{
    if (tightBeforeBufSize) {
        free (tightBeforeBuf);
        tightBeforeBufSize = 0;
        tightBeforeBuf = NULL;
    }
    if (tightAfterBufSize) {
        free (tightAfterBuf);
        tightAfterBufSize = 0;
        tightAfterBuf = NULL;
    }
    if (j) tjDestroy(j);
}


/* Prototypes for static functions. */

static rfbBool SendRectEncodingTight(rfbClientPtr cl, int x, int y,
                                     int w, int h);
static void FindBestSolidArea (rfbClientPtr cl, int x, int y, int w, int h,
                               uint32_t colorValue, int *w_ptr, int *h_ptr);
static void ExtendSolidArea   (rfbClientPtr cl, int x, int y, int w, int h,
                               uint32_t colorValue,
                               int *x_ptr, int *y_ptr, int *w_ptr, int *h_ptr);
static rfbBool CheckSolidTile    (rfbClientPtr cl, int x, int y, int w, int h,
                                  uint32_t *colorPtr, rfbBool needSameColor);
static rfbBool CheckSolidTile8   (rfbClientPtr cl, int x, int y, int w, int h,
                                  uint32_t *colorPtr, rfbBool needSameColor);
static rfbBool CheckSolidTile16  (rfbClientPtr cl, int x, int y, int w, int h,
                                  uint32_t *colorPtr, rfbBool needSameColor);
static rfbBool CheckSolidTile32  (rfbClientPtr cl, int x, int y, int w, int h,
                                  uint32_t *colorPtr, rfbBool needSameColor);

static rfbBool SendRectSimple    (rfbClientPtr cl, int x, int y, int w, int h);
static rfbBool SendSubrect       (rfbClientPtr cl, int x, int y, int w, int h);
static rfbBool SendTightHeader   (rfbClientPtr cl, int x, int y, int w, int h);

static rfbBool SendSolidRect     (rfbClientPtr cl);
static rfbBool SendMonoRect      (rfbClientPtr cl, int x, int y, int w, int h);
static rfbBool SendIndexedRect   (rfbClientPtr cl, int x, int y, int w, int h);
static rfbBool SendFullColorRect (rfbClientPtr cl, int x, int y, int w, int h);

static rfbBool CompressData (rfbClientPtr cl, int streamId, int dataLen,
                             int zlibLevel, int zlibStrategy);
static rfbBool SendCompressedData (rfbClientPtr cl, char *buf,
                                   int compressedLen);

static void FillPalette8 (int count);
static void FillPalette16 (int count);
static void FillPalette32 (int count);
static void FastFillPalette16 (rfbClientPtr cl, uint16_t *data, int w,
                               int pitch, int h);
static void FastFillPalette32 (rfbClientPtr cl, uint32_t *data, int w,
                               int pitch, int h);

static void PaletteReset (void);
static int PaletteInsert (uint32_t rgb, int numPixels, int bpp);

static void Pack24 (rfbClientPtr cl, char *buf, rfbPixelFormat *fmt,
                    int count);

static void EncodeIndexedRect16 (uint8_t *buf, int count);
static void EncodeIndexedRect32 (uint8_t *buf, int count);

static void EncodeMonoRect8 (uint8_t *buf, int w, int h);
static void EncodeMonoRect16 (uint8_t *buf, int w, int h);
static void EncodeMonoRect32 (uint8_t *buf, int w, int h);

static rfbBool SendJpegRect (rfbClientPtr cl, int x, int y, int w, int h,
                             int quality);
static void PrepareRowForImg(rfbClientPtr cl, uint8_t *dst, int x, int y, int count);
static void PrepareRowForImg24(rfbClientPtr cl, uint8_t *dst, int x, int y, int count);
static void PrepareRowForImg16(rfbClientPtr cl, uint8_t *dst, int x, int y, int count);
static void PrepareRowForImg32(rfbClientPtr cl, uint8_t *dst, int x, int y, int count);

#ifdef LIBVNCSERVER_HAVE_LIBPNG
static rfbBool SendPngRect(rfbClientPtr cl, int x, int y, int w, int h);
static rfbBool CanSendPngRect(rfbClientPtr cl, int w, int h);
#endif

/*
 * Tight encoding implementation.
 */

int
rfbNumCodedRectsTight(rfbClientPtr cl,
                      int x,
                      int y,
                      int w,
                      int h)
{
    int maxRectSize, maxRectWidth;
    int subrectMaxWidth, subrectMaxHeight;

    /* No matter how many rectangles we will send if LastRect markers
       are used to terminate rectangle stream. */
    if (cl->enableLastRectEncoding && w * h >= MIN_SPLIT_RECT_SIZE)
        return 0;

    maxRectSize = tightConf[compressLevel].maxRectSize;
    maxRectWidth = tightConf[compressLevel].maxRectWidth;

    if (w > maxRectWidth || w * h > maxRectSize) {
        subrectMaxWidth = (w > maxRectWidth) ? maxRectWidth : w;
        subrectMaxHeight = maxRectSize / subrectMaxWidth;
        return (((w - 1) / maxRectWidth + 1) *
                ((h - 1) / subrectMaxHeight + 1));
    } else {
        return 1;
    }
}

rfbBool
rfbSendRectEncodingTight(rfbClientPtr cl,
                         int x,
                         int y,
                         int w,
                         int h)
{
    cl->tightEncoding = rfbEncodingTight;
    return SendRectEncodingTight(cl, x, y, w, h);
}

rfbBool
rfbSendRectEncodingTightPng(rfbClientPtr cl,
                         int x,
                         int y,
                         int w,
                         int h)
{
    cl->tightEncoding = rfbEncodingTightPng;
    return SendRectEncodingTight(cl, x, y, w, h);
}


rfbBool
SendRectEncodingTight(rfbClientPtr cl,
                         int x,
                         int y,
                         int w,
                         int h)
{
    int nMaxRows;
    uint32_t colorValue;
    int dx, dy, dw, dh;
    int x_best, y_best, w_best, h_best;
    char *fbptr;

    rfbSendUpdateBuf(cl);

    compressLevel = cl->tightCompressLevel;
    qualityLevel = cl->turboQualityLevel;
    subsampLevel = cl->turboSubsampLevel;

    /* We only allow compression levels that have a demonstrable performance
       benefit.  CL 0 with JPEG reduces CPU usage for workloads that have low
       numbers of unique colors, but the same thing can be accomplished by
       using CL 0 without JPEG (AKA "Lossless Tight.")  For those same
       low-color workloads, CL 2 can provide typically 20-40% better
       compression than CL 1 (with a commensurate increase in CPU usage.)  For
       high-color workloads, CL 1 should always be used, as higher compression
       levels increase CPU usage for these workloads without providing any
       significant reduction in bandwidth. */
    if (qualityLevel != -1) {
        if (compressLevel < 1) compressLevel = 1;
        if (compressLevel > 2) compressLevel = 2;
    }

    /* With JPEG disabled, CL 2 offers no significant bandwidth savings over
       CL 1, so we don't include it. */
    else if (compressLevel > 1) compressLevel = 1;

    /* CL 9 (which maps internally to CL 3) is included mainly for backward
       compatibility with TightVNC Compression Levels 5-9.  It should be used
       only in extremely low-bandwidth cases in which it can be shown to have a
       benefit.  For low-color workloads, it provides typically only 10-20%
       better compression than CL 2 with JPEG and CL 1 without JPEG, and it
       uses, on average, twice as much CPU time. */
    if (cl->tightCompressLevel == 9) compressLevel = 3;

    if ( cl->format.depth == 24 && cl->format.redMax == 0xFF &&
         cl->format.greenMax == 0xFF && cl->format.blueMax == 0xFF ) {
        usePixelFormat24 = TRUE;
    } else {
        usePixelFormat24 = FALSE;
    }

    if (!cl->enableLastRectEncoding || w * h < MIN_SPLIT_RECT_SIZE)
        return SendRectSimple(cl, x, y, w, h);

    /* Make sure we can write at least one pixel into tightBeforeBuf. */

    if (tightBeforeBufSize < 4) {
        tightBeforeBufSize = 4;
        if (tightBeforeBuf == NULL)
            tightBeforeBuf = (char *)malloc(tightBeforeBufSize);
        else
            tightBeforeBuf = (char *)realloc(tightBeforeBuf,
                                             tightBeforeBufSize);
    }

    /* Calculate maximum number of rows in one non-solid rectangle. */

    {
        int maxRectSize, maxRectWidth, nMaxWidth;

        maxRectSize = tightConf[compressLevel].maxRectSize;
        maxRectWidth = tightConf[compressLevel].maxRectWidth;
        nMaxWidth = (w > maxRectWidth) ? maxRectWidth : w;
        nMaxRows = maxRectSize / nMaxWidth;
    }

    /* Try to find large solid-color areas and send them separately. */

    for (dy = y; dy < y + h; dy += MAX_SPLIT_TILE_SIZE) {

        /* If a rectangle becomes too large, send its upper part now. */

        if (dy - y >= nMaxRows) {
            if (!SendRectSimple(cl, x, y, w, nMaxRows))
                return 0;
            y += nMaxRows;
            h -= nMaxRows;
        }

        dh = (dy + MAX_SPLIT_TILE_SIZE <= y + h) ?
             MAX_SPLIT_TILE_SIZE : (y + h - dy);

        for (dx = x; dx < x + w; dx += MAX_SPLIT_TILE_SIZE) {

            dw = (dx + MAX_SPLIT_TILE_SIZE <= x + w) ?
                 MAX_SPLIT_TILE_SIZE : (x + w - dx);

            if (CheckSolidTile(cl, dx, dy, dw, dh, &colorValue, FALSE)) {

                if (subsampLevel == TJ_GRAYSCALE && qualityLevel != -1) {
                    uint32_t r = (colorValue >> 16) & 0xFF;
                    uint32_t g = (colorValue >> 8) & 0xFF;
                    uint32_t b = (colorValue) & 0xFF;
                    double y = (0.257 * (double)r) + (0.504 * (double)g)
                             + (0.098 * (double)b) + 16.;
                    colorValue = (int)y + (((int)y) << 8) + (((int)y) << 16);
                }

                /* Get dimensions of solid-color area. */

                FindBestSolidArea(cl, dx, dy, w - (dx - x), h - (dy - y),
				  colorValue, &w_best, &h_best);

                /* Make sure a solid rectangle is large enough
                   (or the whole rectangle is of the same color). */

                if ( w_best * h_best != w * h &&
                     w_best * h_best < MIN_SOLID_SUBRECT_SIZE )
                    continue;

                /* Try to extend solid rectangle to maximum size. */

                x_best = dx; y_best = dy;
                ExtendSolidArea(cl, x, y, w, h, colorValue,
                                &x_best, &y_best, &w_best, &h_best);

                /* Send rectangles at top and left to solid-color area. */

                if ( y_best != y &&
                     !SendRectSimple(cl, x, y, w, y_best-y) )
                    return FALSE;
                if ( x_best != x &&
                     !SendRectEncodingTight(cl, x, y_best,
                                               x_best-x, h_best) )
                    return FALSE;

                /* Send solid-color rectangle. */

                if (!SendTightHeader(cl, x_best, y_best, w_best, h_best))
                    return FALSE;

                fbptr = (cl->scaledScreen->frameBuffer +
                         (cl->scaledScreen->paddedWidthInBytes * y_best) +
                         (x_best * (cl->scaledScreen->bitsPerPixel / 8)));

                (*cl->translateFn)(cl->translateLookupTable, &cl->screen->serverFormat,
                                   &cl->format, fbptr, tightBeforeBuf,
                                   cl->scaledScreen->paddedWidthInBytes, 1, 1);

                if (!SendSolidRect(cl))
                    return FALSE;

                /* Send remaining rectangles (at right and bottom). */

                if ( x_best + w_best != x + w &&
                     !SendRectEncodingTight(cl, x_best + w_best, y_best,
                                               w - (x_best-x) - w_best, h_best) )
                    return FALSE;
                if ( y_best + h_best != y + h &&
                     !SendRectEncodingTight(cl, x, y_best + h_best,
                                               w, h - (y_best-y) - h_best) )
                    return FALSE;

                /* Return after all recursive calls are done. */

                return TRUE;
            }

        }

    }

    /* No suitable solid-color rectangles found. */

    return SendRectSimple(cl, x, y, w, h);
}


static void
FindBestSolidArea(rfbClientPtr cl,
                  int x,
                  int y,
                  int w,
                  int h,
                  uint32_t colorValue,
                  int *w_ptr,
                  int *h_ptr)
{
    int dx, dy, dw, dh;
    int w_prev;
    int w_best = 0, h_best = 0;

    w_prev = w;

    for (dy = y; dy < y + h; dy += MAX_SPLIT_TILE_SIZE) {

        dh = (dy + MAX_SPLIT_TILE_SIZE <= y + h) ?
             MAX_SPLIT_TILE_SIZE : (y + h - dy);
        dw = (w_prev > MAX_SPLIT_TILE_SIZE) ?
             MAX_SPLIT_TILE_SIZE : w_prev;

        if (!CheckSolidTile(cl, x, dy, dw, dh, &colorValue, TRUE))
            break;

        for (dx = x + dw; dx < x + w_prev;) {
            dw = (dx + MAX_SPLIT_TILE_SIZE <= x + w_prev) ?
                 MAX_SPLIT_TILE_SIZE : (x + w_prev - dx);
            if (!CheckSolidTile(cl, dx, dy, dw, dh, &colorValue, TRUE))
                break;
	    dx += dw;
        }

        w_prev = dx - x;
        if (w_prev * (dy + dh - y) > w_best * h_best) {
            w_best = w_prev;
            h_best = dy + dh - y;
        }
    }

    *w_ptr = w_best;
    *h_ptr = h_best;
}


static void
ExtendSolidArea(rfbClientPtr cl,
                int x,
                int y,
                int w,
                int h,
                uint32_t colorValue,
                int *x_ptr,
                int *y_ptr,
                int *w_ptr,
                int *h_ptr)
{
    int cx, cy;

    /* Try to extend the area upwards. */
    for ( cy = *y_ptr - 1;
          cy >= y && CheckSolidTile(cl, *x_ptr, cy, *w_ptr, 1, &colorValue, TRUE);
          cy-- );
    *h_ptr += *y_ptr - (cy + 1);
    *y_ptr = cy + 1;

    /* ... downwards. */
    for ( cy = *y_ptr + *h_ptr;
          cy < y + h &&
              CheckSolidTile(cl, *x_ptr, cy, *w_ptr, 1, &colorValue, TRUE);
          cy++ );
    *h_ptr += cy - (*y_ptr + *h_ptr);

    /* ... to the left. */
    for ( cx = *x_ptr - 1;
          cx >= x && CheckSolidTile(cl, cx, *y_ptr, 1, *h_ptr, &colorValue, TRUE);
          cx-- );
    *w_ptr += *x_ptr - (cx + 1);
    *x_ptr = cx + 1;

    /* ... to the right. */
    for ( cx = *x_ptr + *w_ptr;
          cx < x + w &&
              CheckSolidTile(cl, cx, *y_ptr, 1, *h_ptr, &colorValue, TRUE);
          cx++ );
    *w_ptr += cx - (*x_ptr + *w_ptr);
}


/*
 * Check if a rectangle is all of the same color. If needSameColor is
 * set to non-zero, then also check that its color equals to the
 * *colorPtr value. The result is 1 if the test is successfull, and in
 * that case new color will be stored in *colorPtr.
 */

static rfbBool CheckSolidTile(rfbClientPtr cl, int x, int y, int w, int h, uint32_t* colorPtr, rfbBool needSameColor)
{
    switch(cl->screen->serverFormat.bitsPerPixel) {
    case 32:
        return CheckSolidTile32(cl, x, y, w, h, colorPtr, needSameColor);
    case 16:
        return CheckSolidTile16(cl, x, y, w, h, colorPtr, needSameColor);
    default:
        return CheckSolidTile8(cl, x, y, w, h, colorPtr, needSameColor);
    }
}


#define DEFINE_CHECK_SOLID_FUNCTION(bpp)                                      \
                                                                              \
static rfbBool                                                                \
CheckSolidTile##bpp(rfbClientPtr cl, int x, int y, int w, int h,              \
		uint32_t* colorPtr, rfbBool needSameColor)                    \
{                                                                             \
    uint##bpp##_t *fbptr;                                                     \
    uint##bpp##_t colorValue;                                                 \
    int dx, dy;                                                               \
                                                                              \
    fbptr = (uint##bpp##_t *)&cl->scaledScreen->frameBuffer                   \
        [y * cl->scaledScreen->paddedWidthInBytes + x * (bpp/8)];             \
                                                                              \
    colorValue = *fbptr;                                                      \
    if (needSameColor && (uint32_t)colorValue != *colorPtr)                   \
        return FALSE;                                                         \
                                                                              \
    for (dy = 0; dy < h; dy++) {                                              \
        for (dx = 0; dx < w; dx++) {                                          \
            if (colorValue != fbptr[dx])                                      \
                return FALSE;                                                 \
        }                                                                     \
        fbptr = (uint##bpp##_t *)((uint8_t *)fbptr                            \
                 + cl->scaledScreen->paddedWidthInBytes);                     \
    }                                                                         \
                                                                              \
    *colorPtr = (uint32_t)colorValue;                                         \
    return TRUE;                                                              \
}

DEFINE_CHECK_SOLID_FUNCTION(8)
DEFINE_CHECK_SOLID_FUNCTION(16)
DEFINE_CHECK_SOLID_FUNCTION(32)

static rfbBool
SendRectSimple(rfbClientPtr cl, int x, int y, int w, int h)
{
    int maxBeforeSize, maxAfterSize;
    int maxRectSize, maxRectWidth;
    int subrectMaxWidth, subrectMaxHeight;
    int dx, dy;
    int rw, rh;

    maxRectSize = tightConf[compressLevel].maxRectSize;
    maxRectWidth = tightConf[compressLevel].maxRectWidth;

    maxBeforeSize = maxRectSize * (cl->format.bitsPerPixel / 8);
    maxAfterSize = maxBeforeSize + (maxBeforeSize + 99) / 100 + 12;

    if (tightBeforeBufSize < maxBeforeSize) {
        tightBeforeBufSize = maxBeforeSize;
        if (tightBeforeBuf == NULL)
            tightBeforeBuf = (char *)malloc(tightBeforeBufSize);
        else
            tightBeforeBuf = (char *)realloc(tightBeforeBuf,
                                             tightBeforeBufSize);
    }

    if (tightAfterBufSize < maxAfterSize) {
        tightAfterBufSize = maxAfterSize;
        if (tightAfterBuf == NULL)
            tightAfterBuf = (char *)malloc(tightAfterBufSize);
        else
            tightAfterBuf = (char *)realloc(tightAfterBuf,
                                            tightAfterBufSize);
    }

    if (w > maxRectWidth || w * h > maxRectSize) {
        subrectMaxWidth = (w > maxRectWidth) ? maxRectWidth : w;
        subrectMaxHeight = maxRectSize / subrectMaxWidth;

        for (dy = 0; dy < h; dy += subrectMaxHeight) {
            for (dx = 0; dx < w; dx += maxRectWidth) {
                rw = (dx + maxRectWidth < w) ? maxRectWidth : w - dx;
                rh = (dy + subrectMaxHeight < h) ? subrectMaxHeight : h - dy;
                if (!SendSubrect(cl, x + dx, y + dy, rw, rh))
                    return FALSE;
            }
        }
    } else {
        if (!SendSubrect(cl, x, y, w, h))
            return FALSE;
    }

    return TRUE;
}

static rfbBool
SendSubrect(rfbClientPtr cl,
            int x,
            int y,
            int w,
            int h)
{
    char *fbptr;
    rfbBool success = FALSE;

    /* Send pending data if there is more than 128 bytes. */
    if (cl->ublen > 128) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    if (!SendTightHeader(cl, x, y, w, h))
        return FALSE;

    fbptr = (cl->scaledScreen->frameBuffer
             + (cl->scaledScreen->paddedWidthInBytes * y)
             + (x * (cl->scaledScreen->bitsPerPixel / 8)));

    if (subsampLevel == TJ_GRAYSCALE && qualityLevel != -1)
        return SendJpegRect(cl, x, y, w, h, qualityLevel);

    paletteMaxColors = w * h / tightConf[compressLevel].idxMaxColorsDivisor;
    if(qualityLevel != -1)
        paletteMaxColors = tightConf[compressLevel].palMaxColorsWithJPEG;
    if ( paletteMaxColors < 2 &&
         w * h >= tightConf[compressLevel].monoMinRectSize ) {
        paletteMaxColors = 2;
    }

    if (cl->format.bitsPerPixel == cl->screen->serverFormat.bitsPerPixel &&
        cl->format.redMax == cl->screen->serverFormat.redMax &&
        cl->format.greenMax == cl->screen->serverFormat.greenMax && 
        cl->format.blueMax == cl->screen->serverFormat.blueMax &&
        cl->format.bitsPerPixel >= 16) {

        /* This is so we can avoid translating the pixels when compressing
           with JPEG, since it is unnecessary */
        switch (cl->format.bitsPerPixel) {
        case 16:
            FastFillPalette16(cl, (uint16_t *)fbptr, w,
                              cl->scaledScreen->paddedWidthInBytes / 2, h);
            break;
        default:
            FastFillPalette32(cl, (uint32_t *)fbptr, w,
                              cl->scaledScreen->paddedWidthInBytes / 4, h);
        }

        if(paletteNumColors != 0 || qualityLevel == -1) {
            (*cl->translateFn)(cl->translateLookupTable,
                               &cl->screen->serverFormat, &cl->format, fbptr,
                               tightBeforeBuf,
                               cl->scaledScreen->paddedWidthInBytes, w, h);
        }
    }
    else {
        (*cl->translateFn)(cl->translateLookupTable, &cl->screen->serverFormat,
                           &cl->format, fbptr, tightBeforeBuf,
                           cl->scaledScreen->paddedWidthInBytes, w, h);

        switch (cl->format.bitsPerPixel) {
        case 8:
            FillPalette8(w * h);
            break;
        case 16:
            FillPalette16(w * h);
            break;
        default:
            FillPalette32(w * h);
        }
    }

    switch (paletteNumColors) {
    case 0:
        /* Truecolor image */
        if (qualityLevel != -1) {
            success = SendJpegRect(cl, x, y, w, h, qualityLevel);
        } else {
            success = SendFullColorRect(cl, x, y, w, h);
        }
        break;
    case 1:
        /* Solid rectangle */
        success = SendSolidRect(cl);
        break;
    case 2:
        /* Two-color rectangle */
        success = SendMonoRect(cl, x, y, w, h);
        break;
    default:
        /* Up to 256 different colors */
        success = SendIndexedRect(cl, x, y, w, h);
    }
    return success;
}

static rfbBool
SendTightHeader(rfbClientPtr cl,
                int x,
                int y,
                int w,
                int h)
{
    rfbFramebufferUpdateRectHeader rect;

    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    rect.r.x = Swap16IfLE(x);
    rect.r.y = Swap16IfLE(y);
    rect.r.w = Swap16IfLE(w);
    rect.r.h = Swap16IfLE(h);
    rect.encoding = Swap32IfLE(cl->tightEncoding);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
           sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    rfbStatRecordEncodingSent(cl, cl->tightEncoding,
                              sz_rfbFramebufferUpdateRectHeader,
                              sz_rfbFramebufferUpdateRectHeader
                                  + w * (cl->format.bitsPerPixel / 8) * h);

    return TRUE;
}

/*
 * Subencoding implementations.
 */

static rfbBool
SendSolidRect(rfbClientPtr cl)
{
    int len;

    if (usePixelFormat24) {
        Pack24(cl, tightBeforeBuf, &cl->format, 1);
        len = 3;
    } else
        len = cl->format.bitsPerPixel / 8;

    if (cl->ublen + 1 + len > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    cl->updateBuf[cl->ublen++] = (char)(rfbTightFill << 4);
    memcpy (&cl->updateBuf[cl->ublen], tightBeforeBuf, len);
    cl->ublen += len;

    rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding, len + 1);

    return TRUE;
}

static rfbBool
SendMonoRect(rfbClientPtr cl,
             int x,
             int y,
             int w,
             int h)
{
    int streamId = 1;
    int paletteLen, dataLen;

#ifdef LIBVNCSERVER_HAVE_LIBPNG
    if (CanSendPngRect(cl, w, h)) {
        /* TODO: setup palette maybe */
        return SendPngRect(cl, x, y, w, h);
        /* TODO: destroy palette maybe */
    }
#endif

    if ( cl->ublen + TIGHT_MIN_TO_COMPRESS + 6 +
	 2 * cl->format.bitsPerPixel / 8 > UPDATE_BUF_SIZE ) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    /* Prepare tight encoding header. */
    dataLen = (w + 7) / 8;
    dataLen *= h;

    if (tightConf[compressLevel].monoZlibLevel == 0 &&
        cl->tightEncoding != rfbEncodingTightPng)
        cl->updateBuf[cl->ublen++] =
            (char)((rfbTightNoZlib | rfbTightExplicitFilter) << 4);
    else
        cl->updateBuf[cl->ublen++] = (streamId | rfbTightExplicitFilter) << 4;
    cl->updateBuf[cl->ublen++] = rfbTightFilterPalette;
    cl->updateBuf[cl->ublen++] = 1;

    /* Prepare palette, convert image. */
    switch (cl->format.bitsPerPixel) {

    case 32:
        EncodeMonoRect32((uint8_t *)tightBeforeBuf, w, h);

        ((uint32_t *)tightAfterBuf)[0] = monoBackground;
        ((uint32_t *)tightAfterBuf)[1] = monoForeground;
        if (usePixelFormat24) {
            Pack24(cl, tightAfterBuf, &cl->format, 2);
            paletteLen = 6;
        } else
            paletteLen = 8;

        memcpy(&cl->updateBuf[cl->ublen], tightAfterBuf, paletteLen);
        cl->ublen += paletteLen;
        rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding, 3 + paletteLen);
        break;

    case 16:
        EncodeMonoRect16((uint8_t *)tightBeforeBuf, w, h);

        ((uint16_t *)tightAfterBuf)[0] = (uint16_t)monoBackground;
        ((uint16_t *)tightAfterBuf)[1] = (uint16_t)monoForeground;

        memcpy(&cl->updateBuf[cl->ublen], tightAfterBuf, 4);
        cl->ublen += 4;
        rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding, 7);
        break;

    default:
        EncodeMonoRect8((uint8_t *)tightBeforeBuf, w, h);

        cl->updateBuf[cl->ublen++] = (char)monoBackground;
        cl->updateBuf[cl->ublen++] = (char)monoForeground;
        rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding, 5);
    }

    return CompressData(cl, streamId, dataLen,
                        tightConf[compressLevel].monoZlibLevel,
                        Z_DEFAULT_STRATEGY);
}

static rfbBool
SendIndexedRect(rfbClientPtr cl,
                int x,
                int y,
                int w,
                int h)
{
    int streamId = 2;
    int i, entryLen;

#ifdef LIBVNCSERVER_HAVE_LIBPNG
    if (CanSendPngRect(cl, w, h)) {
        return SendPngRect(cl, x, y, w, h);
    }
#endif

    if ( cl->ublen + TIGHT_MIN_TO_COMPRESS + 6 +
	 paletteNumColors * cl->format.bitsPerPixel / 8 >
         UPDATE_BUF_SIZE ) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    /* Prepare tight encoding header. */
    if (tightConf[compressLevel].idxZlibLevel == 0 &&
        cl->tightEncoding != rfbEncodingTightPng)
        cl->updateBuf[cl->ublen++] =
            (char)((rfbTightNoZlib | rfbTightExplicitFilter) << 4);
    else
        cl->updateBuf[cl->ublen++] = (streamId | rfbTightExplicitFilter) << 4;
    cl->updateBuf[cl->ublen++] = rfbTightFilterPalette;
    cl->updateBuf[cl->ublen++] = (char)(paletteNumColors - 1);

    /* Prepare palette, convert image. */
    switch (cl->format.bitsPerPixel) {

    case 32:
        EncodeIndexedRect32((uint8_t *)tightBeforeBuf, w * h);

        for (i = 0; i < paletteNumColors; i++) {
            ((uint32_t *)tightAfterBuf)[i] =
                palette.entry[i].listNode->rgb;
        }
        if (usePixelFormat24) {
            Pack24(cl, tightAfterBuf, &cl->format, paletteNumColors);
            entryLen = 3;
        } else
            entryLen = 4;

        memcpy(&cl->updateBuf[cl->ublen], tightAfterBuf,
               paletteNumColors * entryLen);
        cl->ublen += paletteNumColors * entryLen;
        rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding,
                                     3 + paletteNumColors * entryLen);
        break;

    case 16:
        EncodeIndexedRect16((uint8_t *)tightBeforeBuf, w * h);

        for (i = 0; i < paletteNumColors; i++) {
            ((uint16_t *)tightAfterBuf)[i] =
                (uint16_t)palette.entry[i].listNode->rgb;
        }

        memcpy(&cl->updateBuf[cl->ublen], tightAfterBuf, paletteNumColors * 2);
        cl->ublen += paletteNumColors * 2;
        rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding,
                                     3 + paletteNumColors * 2);
        break;

    default:
        return FALSE;           /* Should never happen. */
    }

    return CompressData(cl, streamId, w * h,
                        tightConf[compressLevel].idxZlibLevel,
                        Z_DEFAULT_STRATEGY);
}

static rfbBool
SendFullColorRect(rfbClientPtr cl,
                  int x,
                  int y,
                  int w,
                  int h)
{
    int streamId = 0;
    int len;

#ifdef LIBVNCSERVER_HAVE_LIBPNG
    if (CanSendPngRect(cl, w, h)) {
        return SendPngRect(cl, x, y, w, h);
    }
#endif

    if (cl->ublen + TIGHT_MIN_TO_COMPRESS + 1 > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    if (tightConf[compressLevel].rawZlibLevel == 0 &&
        cl->tightEncoding != rfbEncodingTightPng)
        cl->updateBuf[cl->ublen++] = (char)(rfbTightNoZlib << 4);
    else
        cl->updateBuf[cl->ublen++] = 0x00;  /* stream id = 0, no flushing, no filter */
    rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding, 1);

    if (usePixelFormat24) {
        Pack24(cl, tightBeforeBuf, &cl->format, w * h);
        len = 3;
    } else
        len = cl->format.bitsPerPixel / 8;

    return CompressData(cl, streamId, w * h * len,
                        tightConf[compressLevel].rawZlibLevel,
                        Z_DEFAULT_STRATEGY);
}

static rfbBool
CompressData(rfbClientPtr cl,
             int streamId,
             int dataLen,
             int zlibLevel,
             int zlibStrategy)
{
    z_streamp pz;
    int err;

    if (dataLen < TIGHT_MIN_TO_COMPRESS) {
        memcpy(&cl->updateBuf[cl->ublen], tightBeforeBuf, dataLen);
        cl->ublen += dataLen;
        rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding, dataLen);
        return TRUE;
    }

    if (zlibLevel == 0)
        return SendCompressedData (cl, tightBeforeBuf, dataLen);

    pz = &cl->zsStruct[streamId];

    /* Initialize compression stream if needed. */
    if (!cl->zsActive[streamId]) {
        pz->zalloc = Z_NULL;
        pz->zfree = Z_NULL;
        pz->opaque = Z_NULL;

        err = deflateInit2 (pz, zlibLevel, Z_DEFLATED, MAX_WBITS,
                            MAX_MEM_LEVEL, zlibStrategy);
        if (err != Z_OK)
            return FALSE;

        cl->zsActive[streamId] = TRUE;
        cl->zsLevel[streamId] = zlibLevel;
    }

    /* Prepare buffer pointers. */
    pz->next_in = (Bytef *)tightBeforeBuf;
    pz->avail_in = dataLen;
    pz->next_out = (Bytef *)tightAfterBuf;
    pz->avail_out = tightAfterBufSize;

    /* Change compression parameters if needed. */
    if (zlibLevel != cl->zsLevel[streamId]) {
        if (deflateParams (pz, zlibLevel, zlibStrategy) != Z_OK) {
            return FALSE;
        }
        cl->zsLevel[streamId] = zlibLevel;
    }

    /* Actual compression. */
    if (deflate(pz, Z_SYNC_FLUSH) != Z_OK ||
        pz->avail_in != 0 || pz->avail_out == 0) {
        return FALSE;
    }

    return SendCompressedData(cl, tightAfterBuf,
                              tightAfterBufSize - pz->avail_out);
}

static rfbBool SendCompressedData(rfbClientPtr cl, char *buf,
                                  int compressedLen)
{
    int i, portionLen;

    cl->updateBuf[cl->ublen++] = compressedLen & 0x7F;
    rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding, 1);
    if (compressedLen > 0x7F) {
        cl->updateBuf[cl->ublen-1] |= 0x80;
        cl->updateBuf[cl->ublen++] = compressedLen >> 7 & 0x7F;
        rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding, 1);
        if (compressedLen > 0x3FFF) {
            cl->updateBuf[cl->ublen-1] |= 0x80;
            cl->updateBuf[cl->ublen++] = compressedLen >> 14 & 0xFF;
            rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding, 1);
        }
    }

    portionLen = UPDATE_BUF_SIZE;
    for (i = 0; i < compressedLen; i += portionLen) {
        if (i + portionLen > compressedLen) {
            portionLen = compressedLen - i;
        }
        if (cl->ublen + portionLen > UPDATE_BUF_SIZE) {
            if (!rfbSendUpdateBuf(cl))
                return FALSE;
        }
        memcpy(&cl->updateBuf[cl->ublen], &buf[i], portionLen);
        cl->ublen += portionLen;
    }
    rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding, compressedLen);

    return TRUE;
}


/*
 * Code to determine how many different colors used in rectangle.
 */

static void
FillPalette8(int count)
{
    uint8_t *data = (uint8_t *)tightBeforeBuf;
    uint8_t c0, c1;
    int i, n0, n1;

    paletteNumColors = 0;

    c0 = data[0];
    for (i = 1; i < count && data[i] == c0; i++);
    if (i == count) {
        paletteNumColors = 1;
        return;                 /* Solid rectangle */
    }

    if (paletteMaxColors < 2)
        return;

    n0 = i;
    c1 = data[i];
    n1 = 0;
    for (i++; i < count; i++) {
        if (data[i] == c0) {
            n0++;
        } else if (data[i] == c1) {
            n1++;
        } else
            break;
    }
    if (i == count) {
        if (n0 > n1) {
            monoBackground = (uint32_t)c0;
            monoForeground = (uint32_t)c1;
        } else {
            monoBackground = (uint32_t)c1;
            monoForeground = (uint32_t)c0;
        }
        paletteNumColors = 2;   /* Two colors */
    }
}


#define DEFINE_FILL_PALETTE_FUNCTION(bpp)                               \
                                                                        \
static void                                                             \
FillPalette##bpp(int count) {                                           \
    uint##bpp##_t *data = (uint##bpp##_t *)tightBeforeBuf;              \
    uint##bpp##_t c0, c1, ci;                                           \
    int i, n0, n1, ni;                                                  \
                                                                        \
    c0 = data[0];                                                       \
    for (i = 1; i < count && data[i] == c0; i++);                       \
    if (i >= count) {                                                   \
        paletteNumColors = 1;   /* Solid rectangle */                   \
        return;                                                         \
    }                                                                   \
                                                                        \
    if (paletteMaxColors < 2) {                                         \
        paletteNumColors = 0;   /* Full-color encoding preferred */     \
        return;                                                         \
    }                                                                   \
                                                                        \
    n0 = i;                                                             \
    c1 = data[i];                                                       \
    n1 = 0;                                                             \
    for (i++; i < count; i++) {                                         \
        ci = data[i];                                                   \
        if (ci == c0) {                                                 \
            n0++;                                                       \
        } else if (ci == c1) {                                          \
            n1++;                                                       \
        } else                                                          \
            break;                                                      \
    }                                                                   \
    if (i >= count) {                                                   \
        if (n0 > n1) {                                                  \
            monoBackground = (uint32_t)c0;                              \
            monoForeground = (uint32_t)c1;                              \
        } else {                                                        \
            monoBackground = (uint32_t)c1;                              \
            monoForeground = (uint32_t)c0;                              \
        }                                                               \
        paletteNumColors = 2;   /* Two colors */                        \
        return;                                                         \
    }                                                                   \
                                                                        \
    PaletteReset();                                                     \
    PaletteInsert (c0, (uint32_t)n0, bpp);                              \
    PaletteInsert (c1, (uint32_t)n1, bpp);                              \
                                                                        \
    ni = 1;                                                             \
    for (i++; i < count; i++) {                                         \
        if (data[i] == ci) {                                            \
            ni++;                                                       \
        } else {                                                        \
            if (!PaletteInsert (ci, (uint32_t)ni, bpp))                 \
                return;                                                 \
            ci = data[i];                                               \
            ni = 1;                                                     \
        }                                                               \
    }                                                                   \
    PaletteInsert (ci, (uint32_t)ni, bpp);                              \
}

DEFINE_FILL_PALETTE_FUNCTION(16)
DEFINE_FILL_PALETTE_FUNCTION(32)

#define DEFINE_FAST_FILL_PALETTE_FUNCTION(bpp)                          \
                                                                        \
static void                                                             \
FastFillPalette##bpp(rfbClientPtr cl, uint##bpp##_t *data, int w,       \
                     int pitch, int h)                                  \
{                                                                       \
    uint##bpp##_t c0, c1, ci, mask, c0t, c1t, cit;                      \
    int i, j, i2 = 0, j2, n0, n1, ni;                                   \
                                                                        \
    if (cl->translateFn != rfbTranslateNone) {                          \
        mask = cl->screen->serverFormat.redMax                          \
            << cl->screen->serverFormat.redShift;                       \
        mask |= cl->screen->serverFormat.greenMax                       \
             << cl->screen->serverFormat.greenShift;                    \
        mask |= cl->screen->serverFormat.blueMax                        \
             << cl->screen->serverFormat.blueShift;                     \
    } else mask = ~0;                                                   \
                                                                        \
    c0 = data[0] & mask;                                                \
    for (j = 0; j < h; j++) {                                           \
        for (i = 0; i < w; i++) {                                       \
            if ((data[j * pitch + i] & mask) != c0)                     \
                goto done;                                              \
        }                                                               \
    }                                                                   \
    done:                                                               \
    if (j >= h) {                                                       \
        paletteNumColors = 1;   /* Solid rectangle */                   \
        return;                                                         \
    }                                                                   \
    if (paletteMaxColors < 2) {                                         \
        paletteNumColors = 0;   /* Full-color encoding preferred */     \
        return;                                                         \
    }                                                                   \
                                                                        \
    n0 = j * w + i;                                                     \
    c1 = data[j * pitch + i] & mask;                                    \
    n1 = 0;                                                             \
    i++;  if (i >= w) {i = 0;  j++;}                                    \
    for (j2 = j; j2 < h; j2++) {                                        \
        for (i2 = i; i2 < w; i2++) {                                    \
            ci = data[j2 * pitch + i2] & mask;                          \
            if (ci == c0) {                                             \
                n0++;                                                   \
            } else if (ci == c1) {                                      \
                n1++;                                                   \
            } else                                                      \
                goto done2;                                             \
        }                                                               \
        i = 0;                                                          \
    }                                                                   \
    done2:                                                              \
    (*cl->translateFn)(cl->translateLookupTable,                        \
                       &cl->screen->serverFormat, &cl->format,          \
                       (char *)&c0, (char *)&c0t, bpp/8, 1, 1);         \
    (*cl->translateFn)(cl->translateLookupTable,                        \
                       &cl->screen->serverFormat, &cl->format,          \
                       (char *)&c1, (char *)&c1t, bpp/8, 1, 1);         \
    if (j2 >= h) {                                                      \
        if (n0 > n1) {                                                  \
            monoBackground = (uint32_t)c0t;                             \
            monoForeground = (uint32_t)c1t;                             \
        } else {                                                        \
            monoBackground = (uint32_t)c1t;                             \
            monoForeground = (uint32_t)c0t;                             \
        }                                                               \
        paletteNumColors = 2;   /* Two colors */                        \
        return;                                                         \
    }                                                                   \
                                                                        \
    PaletteReset();                                                     \
    PaletteInsert (c0t, (uint32_t)n0, bpp);                             \
    PaletteInsert (c1t, (uint32_t)n1, bpp);                             \
                                                                        \
    ni = 1;                                                             \
    i2++;  if (i2 >= w) {i2 = 0;  j2++;}                                \
    for (j = j2; j < h; j++) {                                          \
        for (i = i2; i < w; i++) {                                      \
            if ((data[j * pitch + i] & mask) == ci) {                   \
                ni++;                                                   \
            } else {                                                    \
                (*cl->translateFn)(cl->translateLookupTable,            \
                                   &cl->screen->serverFormat,           \
                                   &cl->format, (char *)&ci,            \
                                   (char *)&cit, bpp/8, 1, 1);          \
                if (!PaletteInsert (cit, (uint32_t)ni, bpp))            \
                    return;                                             \
                ci = data[j * pitch + i] & mask;                        \
                ni = 1;                                                 \
            }                                                           \
        }                                                               \
        i2 = 0;                                                         \
    }                                                                   \
                                                                        \
    (*cl->translateFn)(cl->translateLookupTable,                        \
                       &cl->screen->serverFormat, &cl->format,          \
                       (char *)&ci, (char *)&cit, bpp/8, 1, 1);         \
    PaletteInsert (cit, (uint32_t)ni, bpp);                             \
}

DEFINE_FAST_FILL_PALETTE_FUNCTION(16)
DEFINE_FAST_FILL_PALETTE_FUNCTION(32)


/*
 * Functions to operate with palette structures.
 */

#define HASH_FUNC16(rgb) ((int)((((rgb) >> 8) + (rgb)) & 0xFF))
#define HASH_FUNC32(rgb) ((int)((((rgb) >> 16) + ((rgb) >> 8)) & 0xFF))


static void
PaletteReset(void)
{
    paletteNumColors = 0;
    memset(palette.hash, 0, 256 * sizeof(COLOR_LIST *));
}


static int
PaletteInsert(uint32_t rgb,
              int numPixels,
              int bpp)
{
    COLOR_LIST *pnode;
    COLOR_LIST *prev_pnode = NULL;
    int hash_key, idx, new_idx, count;

    hash_key = (bpp == 16) ? HASH_FUNC16(rgb) : HASH_FUNC32(rgb);

    pnode = palette.hash[hash_key];

    while (pnode != NULL) {
        if (pnode->rgb == rgb) {
            /* Such palette entry already exists. */
            new_idx = idx = pnode->idx;
            count = palette.entry[idx].numPixels + numPixels;
            if (new_idx && palette.entry[new_idx-1].numPixels < count) {
                do {
                    palette.entry[new_idx] = palette.entry[new_idx-1];
                    palette.entry[new_idx].listNode->idx = new_idx;
                    new_idx--;
                }
                while (new_idx && palette.entry[new_idx-1].numPixels < count);
                palette.entry[new_idx].listNode = pnode;
                pnode->idx = new_idx;
            }
            palette.entry[new_idx].numPixels = count;
            return paletteNumColors;
        }
        prev_pnode = pnode;
        pnode = pnode->next;
    }

    /* Check if palette is full. */
    if (paletteNumColors == 256 || paletteNumColors == paletteMaxColors) {
        paletteNumColors = 0;
        return 0;
    }

    /* Move palette entries with lesser pixel counts. */
    for ( idx = paletteNumColors;
          idx > 0 && palette.entry[idx-1].numPixels < numPixels;
          idx-- ) {
        palette.entry[idx] = palette.entry[idx-1];
        palette.entry[idx].listNode->idx = idx;
    }

    /* Add new palette entry into the freed slot. */
    pnode = &palette.list[paletteNumColors];
    if (prev_pnode != NULL) {
        prev_pnode->next = pnode;
    } else {
        palette.hash[hash_key] = pnode;
    }
    pnode->next = NULL;
    pnode->idx = idx;
    pnode->rgb = rgb;
    palette.entry[idx].listNode = pnode;
    palette.entry[idx].numPixels = numPixels;

    return (++paletteNumColors);
}


/*
 * Converting 32-bit color samples into 24-bit colors.
 * Should be called only when redMax, greenMax and blueMax are 255.
 * Color components assumed to be byte-aligned.
 */

static void Pack24(rfbClientPtr cl,
                   char *buf,
                   rfbPixelFormat *fmt,
                   int count)
{
    uint32_t *buf32;
    uint32_t pix;
    int r_shift, g_shift, b_shift;

    buf32 = (uint32_t *)buf;

    if (!cl->screen->serverFormat.bigEndian == !fmt->bigEndian) {
        r_shift = fmt->redShift;
        g_shift = fmt->greenShift;
        b_shift = fmt->blueShift;
    } else {
        r_shift = 24 - fmt->redShift;
        g_shift = 24 - fmt->greenShift;
        b_shift = 24 - fmt->blueShift;
    }

    while (count--) {
        pix = *buf32++;
        *buf++ = (char)(pix >> r_shift);
        *buf++ = (char)(pix >> g_shift);
        *buf++ = (char)(pix >> b_shift);
    }
}


/*
 * Converting truecolor samples into palette indices.
 */

#define DEFINE_IDX_ENCODE_FUNCTION(bpp)                                 \
                                                                        \
static void                                                             \
EncodeIndexedRect##bpp(uint8_t *buf, int count) {                       \
    COLOR_LIST *pnode;                                                  \
    uint##bpp##_t *src;                                                 \
    uint##bpp##_t rgb;                                                  \
    int rep = 0;                                                        \
                                                                        \
    src = (uint##bpp##_t *) buf;                                        \
                                                                        \
    while (count--) {                                                   \
        rgb = *src++;                                                   \
        while (count && *src == rgb) {                                  \
            rep++, src++, count--;                                      \
        }                                                               \
        pnode = palette.hash[HASH_FUNC##bpp(rgb)];                      \
        while (pnode != NULL) {                                         \
            if ((uint##bpp##_t)pnode->rgb == rgb) {                     \
                *buf++ = (uint8_t)pnode->idx;                           \
                while (rep) {                                           \
                    *buf++ = (uint8_t)pnode->idx;                       \
                    rep--;                                              \
                }                                                       \
                break;                                                  \
            }                                                           \
            pnode = pnode->next;                                        \
        }                                                               \
    }                                                                   \
}

DEFINE_IDX_ENCODE_FUNCTION(16)
DEFINE_IDX_ENCODE_FUNCTION(32)


#define DEFINE_MONO_ENCODE_FUNCTION(bpp)                                \
                                                                        \
static void                                                             \
EncodeMonoRect##bpp(uint8_t *buf, int w, int h) {                       \
    uint##bpp##_t *ptr;                                                 \
    uint##bpp##_t bg;                                                   \
    unsigned int value, mask;                                           \
    int aligned_width;                                                  \
    int x, y, bg_bits;                                                  \
                                                                        \
    ptr = (uint##bpp##_t *) buf;                                        \
    bg = (uint##bpp##_t) monoBackground;                                \
    aligned_width = w - w % 8;                                          \
                                                                        \
    for (y = 0; y < h; y++) {                                           \
        for (x = 0; x < aligned_width; x += 8) {                        \
            for (bg_bits = 0; bg_bits < 8; bg_bits++) {                 \
                if (*ptr++ != bg)                                       \
                    break;                                              \
            }                                                           \
            if (bg_bits == 8) {                                         \
                *buf++ = 0;                                             \
                continue;                                               \
            }                                                           \
            mask = 0x80 >> bg_bits;                                     \
            value = mask;                                               \
            for (bg_bits++; bg_bits < 8; bg_bits++) {                   \
                mask >>= 1;                                             \
                if (*ptr++ != bg) {                                     \
                    value |= mask;                                      \
                }                                                       \
            }                                                           \
            *buf++ = (uint8_t)value;                                    \
        }                                                               \
                                                                        \
        mask = 0x80;                                                    \
        value = 0;                                                      \
        if (x >= w)                                                     \
            continue;                                                   \
                                                                        \
        for (; x < w; x++) {                                            \
            if (*ptr++ != bg) {                                         \
                value |= mask;                                          \
            }                                                           \
            mask >>= 1;                                                 \
        }                                                               \
        *buf++ = (uint8_t)value;                                        \
    }                                                                   \
}

DEFINE_MONO_ENCODE_FUNCTION(8)
DEFINE_MONO_ENCODE_FUNCTION(16)
DEFINE_MONO_ENCODE_FUNCTION(32)


/*
 * JPEG compression stuff.
 */

static rfbBool
SendJpegRect(rfbClientPtr cl, int x, int y, int w, int h, int quality)
{
    unsigned char *srcbuf;
    int ps = cl->screen->serverFormat.bitsPerPixel / 8;
    int subsamp = subsampLevel2tjsubsamp[subsampLevel];
    unsigned long size = 0;
    int flags = 0, pitch;
    unsigned char *tmpbuf = NULL;

    if (cl->screen->serverFormat.bitsPerPixel == 8)
        return SendFullColorRect(cl, x, y, w, h);

    if (ps < 2) {
        rfbLog("Error: JPEG requires 16-bit, 24-bit, or 32-bit pixel format.\n");
        return 0;
    }
    if (!j) {
        if ((j = tjInitCompress()) == NULL) {
            rfbLog("JPEG Error: %s\n", tjGetErrorStr());
            return 0;
        }
    }

    if (tightAfterBufSize < TJBUFSIZE(w, h)) {
        if (tightAfterBuf == NULL)
            tightAfterBuf = (char *)malloc(TJBUFSIZE(w, h));
        else
            tightAfterBuf = (char *)realloc(tightAfterBuf,
                                            TJBUFSIZE(w, h));
        if (!tightAfterBuf) {
            rfbLog("Memory allocation failure!\n");
            return 0;
        }
        tightAfterBufSize = TJBUFSIZE(w, h);
    }

    if (ps == 2) {
        uint16_t *srcptr, pix;
        unsigned char *dst;
        int inRed, inGreen, inBlue, i, j;

        if((tmpbuf = (unsigned char *)malloc(w * h * 3)) == NULL)
            rfbLog("Memory allocation failure!\n");
        srcptr = (uint16_t *)&cl->scaledScreen->frameBuffer
            [y * cl->scaledScreen->paddedWidthInBytes + x * ps];
        dst = tmpbuf;
        for(j = 0; j < h; j++) {
            uint16_t *srcptr2 = srcptr;
            unsigned char *dst2 = dst;
            for (i = 0; i < w; i++) {
                pix = *srcptr2++;
                inRed = (int) (pix >> cl->screen->serverFormat.redShift
                               & cl->screen->serverFormat.redMax);
                inGreen = (int) (pix >> cl->screen->serverFormat.greenShift
                                 & cl->screen->serverFormat.greenMax);
                inBlue  = (int) (pix >> cl->screen->serverFormat.blueShift
                                 & cl->screen->serverFormat.blueMax);
                *dst2++ = (uint8_t)((inRed * 255
                                     + cl->screen->serverFormat.redMax / 2)
                                    / cl->screen->serverFormat.redMax);
               	*dst2++ = (uint8_t)((inGreen * 255
                                     + cl->screen->serverFormat.greenMax / 2)
                                    / cl->screen->serverFormat.greenMax);
                *dst2++ = (uint8_t)((inBlue * 255
                                     + cl->screen->serverFormat.blueMax / 2)
                                    / cl->screen->serverFormat.blueMax);
            }
            srcptr += cl->scaledScreen->paddedWidthInBytes / ps;
            dst += w * 3;
        }
        srcbuf = tmpbuf;
        pitch = w * 3;
        ps = 3;
    } else {
        if (cl->screen->serverFormat.bigEndian && ps == 4)
            flags |= TJ_ALPHAFIRST;
        if (cl->screen->serverFormat.redShift == 16
            && cl->screen->serverFormat.blueShift == 0)
            flags |= TJ_BGR;
        if (cl->screen->serverFormat.bigEndian)
            flags ^= TJ_BGR;
        pitch = cl->scaledScreen->paddedWidthInBytes;
        srcbuf = (unsigned char *)&cl->scaledScreen->frameBuffer
            [y * pitch + x * ps];
    }

    if (tjCompress(j, srcbuf, w, pitch, h, ps, (unsigned char *)tightAfterBuf,
                   &size, subsamp, quality, flags) == -1) {
        rfbLog("JPEG Error: %s\n", tjGetErrorStr());
        if (tmpbuf) {
            free(tmpbuf);
            tmpbuf = NULL;
        }
        return 0;
    }

    if (tmpbuf) {
        free(tmpbuf);
        tmpbuf = NULL;
    }

    if (cl->ublen + TIGHT_MIN_TO_COMPRESS + 1 > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    cl->updateBuf[cl->ublen++] = (char)(rfbTightJpeg << 4);
    rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding, 1);

    return SendCompressedData(cl, tightAfterBuf, (int)size);
}

static void
PrepareRowForImg(rfbClientPtr cl,
                  uint8_t *dst,
                  int x,
                  int y,
                  int count)
{
    if (cl->screen->serverFormat.bitsPerPixel == 32) {
        if ( cl->screen->serverFormat.redMax == 0xFF &&
             cl->screen->serverFormat.greenMax == 0xFF &&
             cl->screen->serverFormat.blueMax == 0xFF ) {
            PrepareRowForImg24(cl, dst, x, y, count);
        } else {
            PrepareRowForImg32(cl, dst, x, y, count);
        }
    } else {
        /* 16 bpp assumed. */
        PrepareRowForImg16(cl, dst, x, y, count);
    }
}

static void
PrepareRowForImg24(rfbClientPtr cl,
                    uint8_t *dst,
                    int x,
                    int y,
                    int count)
{
    uint32_t *fbptr;
    uint32_t pix;

    fbptr = (uint32_t *)
        &cl->scaledScreen->frameBuffer[y * cl->scaledScreen->paddedWidthInBytes + x * 4];

    while (count--) {
        pix = *fbptr++;
        *dst++ = (uint8_t)(pix >> cl->screen->serverFormat.redShift);
        *dst++ = (uint8_t)(pix >> cl->screen->serverFormat.greenShift);
        *dst++ = (uint8_t)(pix >> cl->screen->serverFormat.blueShift);
    }
}

#define DEFINE_JPEG_GET_ROW_FUNCTION(bpp)                                   \
                                                                            \
static void                                                                 \
PrepareRowForImg##bpp(rfbClientPtr cl, uint8_t *dst, int x, int y, int count) { \
    uint##bpp##_t *fbptr;                                                   \
    uint##bpp##_t pix;                                                      \
    int inRed, inGreen, inBlue;                                             \
                                                                            \
    fbptr = (uint##bpp##_t *)                                               \
        &cl->scaledScreen->frameBuffer[y * cl->scaledScreen->paddedWidthInBytes +       \
                             x * (bpp / 8)];                                \
                                                                            \
    while (count--) {                                                       \
        pix = *fbptr++;                                                     \
                                                                            \
        inRed = (int)                                                       \
            (pix >> cl->screen->serverFormat.redShift   & cl->screen->serverFormat.redMax); \
        inGreen = (int)                                                     \
            (pix >> cl->screen->serverFormat.greenShift & cl->screen->serverFormat.greenMax); \
        inBlue  = (int)                                                     \
            (pix >> cl->screen->serverFormat.blueShift  & cl->screen->serverFormat.blueMax); \
                                                                            \
	*dst++ = (uint8_t)((inRed   * 255 + cl->screen->serverFormat.redMax / 2) / \
                         cl->screen->serverFormat.redMax);                  \
	*dst++ = (uint8_t)((inGreen * 255 + cl->screen->serverFormat.greenMax / 2) / \
                         cl->screen->serverFormat.greenMax);                \
	*dst++ = (uint8_t)((inBlue  * 255 + cl->screen->serverFormat.blueMax / 2) / \
                         cl->screen->serverFormat.blueMax);                 \
    }                                                                       \
}

DEFINE_JPEG_GET_ROW_FUNCTION(16)
DEFINE_JPEG_GET_ROW_FUNCTION(32)

/*
 * PNG compression stuff.
 */

#ifdef LIBVNCSERVER_HAVE_LIBPNG

static TLS int pngDstDataLen = 0;

static rfbBool CanSendPngRect(rfbClientPtr cl, int w, int h) {
    if (cl->tightEncoding != rfbEncodingTightPng) {
        return FALSE;
    }

    if ( cl->screen->serverFormat.bitsPerPixel == 8 ||
         cl->format.bitsPerPixel == 8) {
        return FALSE;
    }

    return TRUE;
}

static void pngWriteData(png_structp png_ptr, png_bytep data,
                           png_size_t length)
{
#if 0
    rfbClientPtr cl = png_get_io_ptr(png_ptr);

    buffer_reserve(&vs->tight.png, vs->tight.png.offset + length);
    memcpy(vs->tight.png.buffer + vs->tight.png.offset, data, length);
#endif
    memcpy(tightAfterBuf + pngDstDataLen, data, length);

    pngDstDataLen += length;
}

static void pngFlushData(png_structp png_ptr)
{
}


static void *pngMalloc(png_structp png_ptr, png_size_t size)
{
    return malloc(size);
}

static void pngFree(png_structp png_ptr, png_voidp ptr)
{
    free(ptr);
}

static rfbBool SendPngRect(rfbClientPtr cl, int x, int y, int w, int h) {
    /* rfbLog(">> SendPngRect x:%d, y:%d, w:%d, h:%d\n", x, y, w, h); */

    png_byte color_type;
    png_structp png_ptr;
    png_infop info_ptr;
    png_colorp png_palette = NULL;
    int level = tightPngConf[cl->tightCompressLevel].png_zlib_level;
    int filters = tightPngConf[cl->tightCompressLevel].png_filters;
    uint8_t *buf;
    int dy;

    pngDstDataLen = 0;

    png_ptr = png_create_write_struct_2(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL,
                                        NULL, pngMalloc, pngFree);

    if (png_ptr == NULL)
        return FALSE;

    info_ptr = png_create_info_struct(png_ptr);

    if (info_ptr == NULL) {
        png_destroy_write_struct(&png_ptr, NULL);
        return FALSE;
    }

    png_set_write_fn(png_ptr, (void *) cl, pngWriteData, pngFlushData);
    png_set_compression_level(png_ptr, level);
    png_set_filter(png_ptr, PNG_FILTER_TYPE_DEFAULT, filters);

#if 0
    /* TODO: */
    if (palette) {
        color_type = PNG_COLOR_TYPE_PALETTE;
    } else {
        color_type = PNG_COLOR_TYPE_RGB;
    }
#else
    color_type = PNG_COLOR_TYPE_RGB;
#endif
    png_set_IHDR(png_ptr, info_ptr, w, h,
                 8, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

#if 0
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        struct palette_cb_priv priv;

        png_palette = pngMalloc(png_ptr, sizeof(*png_palette) *
                                 palette_size(palette));

        priv.vs = vs;
        priv.png_palette = png_palette;
        palette_iter(palette, write_png_palette, &priv);

        png_set_PLTE(png_ptr, info_ptr, png_palette, palette_size(palette));

        offset = vs->tight.tight.offset;
        if (vs->clientds.pf.bytes_per_pixel == 4) {
            tight_encode_indexed_rect32(vs->tight.tight.buffer, w * h, palette);
        } else {
            tight_encode_indexed_rect16(vs->tight.tight.buffer, w * h, palette);
        }
    }

    buffer_reserve(&vs->tight.png, 2048);
#endif

    png_write_info(png_ptr, info_ptr);
    buf = malloc(w * 3);
    for (dy = 0; dy < h; dy++)
    {
#if 0
        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            memcpy(buf, vs->tight.tight.buffer + (dy * w), w);
        } else {
            PrepareRowForImg(cl, buf, x, y + dy, w);
        }
#else
        PrepareRowForImg(cl, buf, x, y + dy, w);
#endif
        png_write_row(png_ptr, buf);
    }
    free(buf);

    png_write_end(png_ptr, NULL);

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        pngFree(png_ptr, png_palette);
    }

    png_destroy_write_struct(&png_ptr, &info_ptr);

    /* done v */

    if (cl->ublen + TIGHT_MIN_TO_COMPRESS + 1 > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    cl->updateBuf[cl->ublen++] = (char)(rfbTightPng << 4);
    rfbStatRecordEncodingSentAdd(cl, cl->tightEncoding, 1);

    /* rfbLog("<< SendPngRect\n"); */
    return SendCompressedData(cl, tightAfterBuf, pngDstDataLen);
}
#endif
