/*
 * tight.c
 *
 * Routines to implement Tight Encoding
 */

/*
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*#include <stdio.h>*/
#include <rfb/rfb.h>
#include "private.h"

#ifdef WIN32
#define XMD_H
#undef FAR
#define NEEDFAR_POINTERS
#endif

#include <jpeglib.h>

/* Note: The following constant should not be changed. */
#define TIGHT_MIN_TO_COMPRESS 12

/* The parameters below may be adjusted. */
#define MIN_SPLIT_RECT_SIZE     4096
#define MIN_SOLID_SUBRECT_SIZE  2048
#define MAX_SPLIT_TILE_SIZE       16

/* May be set to TRUE with "-lazytight" Xvnc option. */
rfbBool rfbTightDisableGradient = FALSE;

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
    int monoMinRectSize, gradientMinRectSize;
    int idxZlibLevel, monoZlibLevel, rawZlibLevel, gradientZlibLevel;
    int gradientThreshold, gradientThreshold24;
    int idxMaxColorsDivisor;
    int jpegQuality, jpegThreshold, jpegThreshold24;
} TIGHT_CONF;

static TIGHT_CONF tightConf[10] = {
    {   512,   32,   6, 65536, 0, 0, 0, 0,   0,   0,   4,  5, 10000, 23000 },
    {  2048,  128,   6, 65536, 1, 1, 1, 0,   0,   0,   8, 10,  8000, 18000 },
    {  6144,  256,   8, 65536, 3, 3, 2, 0,   0,   0,  24, 15,  6500, 15000 },
    { 10240, 1024,  12, 65536, 5, 5, 3, 0,   0,   0,  32, 25,  5000, 12000 },
    { 16384, 2048,  12, 65536, 6, 6, 4, 0,   0,   0,  32, 37,  4000, 10000 },
    { 32768, 2048,  12,  4096, 7, 7, 5, 4, 150, 380,  32, 50,  3000,  8000 },
    { 65536, 2048,  16,  4096, 7, 7, 6, 4, 170, 420,  48, 60,  2000,  5000 },
    { 65536, 2048,  16,  4096, 8, 8, 7, 5, 180, 450,  64, 70,  1000,  2500 },
    { 65536, 2048,  32,  8192, 9, 9, 8, 6, 190, 475,  64, 75,   500,  1200 },
    { 65536, 2048,  32,  8192, 9, 9, 9, 6, 200, 500,  96, 80,   200,   500 }
};

static TLS int compressLevel = 0;
static TLS int qualityLevel = 0;

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

static TLS int *prevRowBuf = NULL;

void rfbTightCleanup(rfbScreenInfoPtr screen)
{
  if(tightBeforeBufSize) {
    free(tightBeforeBuf);
    tightBeforeBufSize=0;
    tightBeforeBuf = NULL;
  }
  if(tightAfterBufSize) {
    free(tightAfterBuf);
    tightAfterBufSize=0;
    tightAfterBuf = NULL;
  }
}

/* Prototypes for static functions. */

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
static rfbBool SendMonoRect      (rfbClientPtr cl, int w, int h);
static rfbBool SendIndexedRect   (rfbClientPtr cl, int w, int h);
static rfbBool SendFullColorRect (rfbClientPtr cl, int w, int h);
static rfbBool SendGradientRect  (rfbClientPtr cl, int w, int h);

static rfbBool CompressData(rfbClientPtr cl, int streamId, int dataLen,
                         int zlibLevel, int zlibStrategy);
static rfbBool SendCompressedData(rfbClientPtr cl, int compressedLen);

static void FillPalette8(int count);
static void FillPalette16(int count);
static void FillPalette32(int count);

static void PaletteReset(void);
static int PaletteInsert(uint32_t rgb, int numPixels, int bpp);

static void Pack24(rfbClientPtr cl, char *buf, rfbPixelFormat *fmt, int count);

static void EncodeIndexedRect16(uint8_t *buf, int count);
static void EncodeIndexedRect32(uint8_t *buf, int count);

static void EncodeMonoRect8(uint8_t *buf, int w, int h);
static void EncodeMonoRect16(uint8_t *buf, int w, int h);
static void EncodeMonoRect32(uint8_t *buf, int w, int h);

static void FilterGradient24(rfbClientPtr cl, char *buf, rfbPixelFormat *fmt, int w, int h);
static void FilterGradient16(rfbClientPtr cl, uint16_t *buf, rfbPixelFormat *fmt, int w, int h);
static void FilterGradient32(rfbClientPtr cl, uint32_t *buf, rfbPixelFormat *fmt, int w, int h);

static int DetectSmoothImage(rfbClientPtr cl, rfbPixelFormat *fmt, int w, int h);
static unsigned long DetectSmoothImage24(rfbClientPtr cl, rfbPixelFormat *fmt, int w, int h);
static unsigned long DetectSmoothImage16(rfbClientPtr cl, rfbPixelFormat *fmt, int w, int h);
static unsigned long DetectSmoothImage32(rfbClientPtr cl, rfbPixelFormat *fmt, int w, int h);

static rfbBool SendJpegRect(rfbClientPtr cl, int x, int y, int w, int h,
                         int quality);
static void PrepareRowForJpeg(rfbClientPtr cl, uint8_t *dst, int x, int y, int count);
static void PrepareRowForJpeg24(rfbClientPtr cl, uint8_t *dst, int x, int y, int count);
static void PrepareRowForJpeg16(rfbClientPtr cl, uint8_t *dst, int x, int y, int count);
static void PrepareRowForJpeg32(rfbClientPtr cl, uint8_t *dst, int x, int y, int count);

static void JpegInitDestination(j_compress_ptr cinfo);
static boolean JpegEmptyOutputBuffer(j_compress_ptr cinfo);
static void JpegTermDestination(j_compress_ptr cinfo);
static void JpegSetDstManager(j_compress_ptr cinfo);


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

    maxRectSize = tightConf[cl->tightCompressLevel].maxRectSize;
    maxRectWidth = tightConf[cl->tightCompressLevel].maxRectWidth;

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
    int nMaxRows;
    uint32_t colorValue;
    int dx, dy, dw, dh;
    int x_best, y_best, w_best, h_best;
    char *fbptr;

    rfbSendUpdateBuf(cl);

    compressLevel = cl->tightCompressLevel;
    qualityLevel = cl->tightQualityLevel;

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
                     !rfbSendRectEncodingTight(cl, x, y_best,
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
                     !rfbSendRectEncodingTight(cl, x_best+w_best, y_best,
                                               w-(x_best-x)-w_best, h_best) )
                    return FALSE;
                if ( y_best + h_best != y + h &&
                     !rfbSendRectEncodingTight(cl, x, y_best+h_best,
                                               w, h-(y_best-y)-h_best) )
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
    fbptr = (uint##bpp##_t *)                                                 \
        &cl->scaledScreen->frameBuffer[y * cl->scaledScreen->paddedWidthInBytes + x * (bpp/8)]; \
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
        fbptr = (uint##bpp##_t *)((uint8_t *)fbptr + cl->scaledScreen->paddedWidthInBytes); \
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
                if (!SendSubrect(cl, x+dx, y+dy, rw, rh))
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

    fbptr = (cl->scaledScreen->frameBuffer + (cl->scaledScreen->paddedWidthInBytes * y)
             + (x * (cl->scaledScreen->bitsPerPixel / 8)));

    (*cl->translateFn)(cl->translateLookupTable, &cl->screen->serverFormat,
                       &cl->format, fbptr, tightBeforeBuf,
                       cl->scaledScreen->paddedWidthInBytes, w, h);

    paletteMaxColors = w * h / tightConf[compressLevel].idxMaxColorsDivisor;
    if ( paletteMaxColors < 2 &&
         w * h >= tightConf[compressLevel].monoMinRectSize ) {
        paletteMaxColors = 2;
    }
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

    switch (paletteNumColors) {
    case 0:
        /* Truecolor image */
        if (DetectSmoothImage(cl, &cl->format, w, h)) {
            if (qualityLevel != -1) {
                success = SendJpegRect(cl, x, y, w, h,
                                       tightConf[qualityLevel].jpegQuality);
            } else {
                success = SendGradientRect(cl, w, h);
            }
        } else {
            success = SendFullColorRect(cl, w, h);
        }
        break;
    case 1:
        /* Solid rectangle */
        success = SendSolidRect(cl);
        break;
    case 2:
        /* Two-color rectangle */
        success = SendMonoRect(cl, w, h);
        break;
    default:
        /* Up to 256 different colors */
        if ( paletteNumColors > 96 &&
             qualityLevel != -1 && qualityLevel <= 3 &&
             DetectSmoothImage(cl, &cl->format, w, h) ) {
            success = SendJpegRect(cl, x, y, w, h,
                                   tightConf[qualityLevel].jpegQuality);
        } else {
            success = SendIndexedRect(cl, w, h);
        }
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
    rect.encoding = Swap32IfLE(rfbEncodingTight);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
           sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    rfbStatRecordEncodingSent(cl, rfbEncodingTight, sz_rfbFramebufferUpdateRectHeader,
                              sz_rfbFramebufferUpdateRectHeader + w * (cl->format.bitsPerPixel / 8) * h);

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

    rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, len+1);

    return TRUE;
}

static rfbBool
SendMonoRect(rfbClientPtr cl,
             int w,
             int h)
{
    int streamId = 1;
    int paletteLen, dataLen;

    if ( cl->ublen + TIGHT_MIN_TO_COMPRESS + 6 +
	 2 * cl->format.bitsPerPixel / 8 > UPDATE_BUF_SIZE ) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    /* Prepare tight encoding header. */
    dataLen = (w + 7) / 8;
    dataLen *= h;

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
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 3 + paletteLen);
        break;

    case 16:
        EncodeMonoRect16((uint8_t *)tightBeforeBuf, w, h);

        ((uint16_t *)tightAfterBuf)[0] = (uint16_t)monoBackground;
        ((uint16_t *)tightAfterBuf)[1] = (uint16_t)monoForeground;

        memcpy(&cl->updateBuf[cl->ublen], tightAfterBuf, 4);
        cl->ublen += 4;
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 7);
        break;

    default:
        EncodeMonoRect8((uint8_t *)tightBeforeBuf, w, h);

        cl->updateBuf[cl->ublen++] = (char)monoBackground;
        cl->updateBuf[cl->ublen++] = (char)monoForeground;
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 5);
    }

    return CompressData(cl, streamId, dataLen,
                        tightConf[compressLevel].monoZlibLevel,
                        Z_DEFAULT_STRATEGY);
}

static rfbBool
SendIndexedRect(rfbClientPtr cl,
                int w,
                int h)
{
    int streamId = 2;
    int i, entryLen;

    if ( cl->ublen + TIGHT_MIN_TO_COMPRESS + 6 +
	 paletteNumColors * cl->format.bitsPerPixel / 8 >
         UPDATE_BUF_SIZE ) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    /* Prepare tight encoding header. */
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

        memcpy(&cl->updateBuf[cl->ublen], tightAfterBuf, paletteNumColors * entryLen);
        cl->ublen += paletteNumColors * entryLen;
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 3 + paletteNumColors * entryLen);
        break;

    case 16:
        EncodeIndexedRect16((uint8_t *)tightBeforeBuf, w * h);

        for (i = 0; i < paletteNumColors; i++) {
            ((uint16_t *)tightAfterBuf)[i] =
                (uint16_t)palette.entry[i].listNode->rgb;
        }

        memcpy(&cl->updateBuf[cl->ublen], tightAfterBuf, paletteNumColors * 2);
        cl->ublen += paletteNumColors * 2;
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 3 + paletteNumColors * 2);
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
                  int w,
                  int h)
{
    int streamId = 0;
    int len;

    if (cl->ublen + TIGHT_MIN_TO_COMPRESS + 1 > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    cl->updateBuf[cl->ublen++] = 0x00;  /* stream id = 0, no flushing, no filter */
    rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 1);

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
SendGradientRect(rfbClientPtr cl,
                 int w,
                 int h)
{
    int streamId = 3;
    int len;

    if (cl->format.bitsPerPixel == 8)
        return SendFullColorRect(cl, w, h);

    if (cl->ublen + TIGHT_MIN_TO_COMPRESS + 2 > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    if (prevRowBuf == NULL)
        prevRowBuf = (int *)malloc(2048 * 3 * sizeof(int));

    cl->updateBuf[cl->ublen++] = (streamId | rfbTightExplicitFilter) << 4;
    cl->updateBuf[cl->ublen++] = rfbTightFilterGradient;
    rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 2);

    if (usePixelFormat24) {
        FilterGradient24(cl, tightBeforeBuf, &cl->format, w, h);
        len = 3;
    } else if (cl->format.bitsPerPixel == 32) {
        FilterGradient32(cl, (uint32_t *)tightBeforeBuf, &cl->format, w, h);
        len = 4;
    } else {
        FilterGradient16(cl, (uint16_t *)tightBeforeBuf, &cl->format, w, h);
        len = 2;
    }

    return CompressData(cl, streamId, w * h * len,
                        tightConf[compressLevel].gradientZlibLevel,
                        Z_FILTERED);
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
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, dataLen);
        return TRUE;
    }

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
    if ( deflate (pz, Z_SYNC_FLUSH) != Z_OK ||
         pz->avail_in != 0 || pz->avail_out == 0 ) {
        return FALSE;
    }

    return SendCompressedData(cl, tightAfterBufSize - pz->avail_out);
}

static rfbBool SendCompressedData(rfbClientPtr cl,
                                  int compressedLen)
{
    int i, portionLen;

    cl->updateBuf[cl->ublen++] = compressedLen & 0x7F;
    rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 1);
    if (compressedLen > 0x7F) {
        cl->updateBuf[cl->ublen-1] |= 0x80;
        cl->updateBuf[cl->ublen++] = compressedLen >> 7 & 0x7F;
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 1);
        if (compressedLen > 0x3FFF) {
            cl->updateBuf[cl->ublen-1] |= 0x80;
            cl->updateBuf[cl->ublen++] = compressedLen >> 14 & 0xFF;
            rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 1);
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
        memcpy(&cl->updateBuf[cl->ublen], &tightAfterBuf[i], portionLen);
        cl->ublen += portionLen;
    }
    rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, compressedLen);

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


/*
 * Functions to operate with palette structures.
 */

#define HASH_FUNC16(rgb) ((int)(((rgb >> 8) + rgb) & 0xFF))
#define HASH_FUNC32(rgb) ((int)(((rgb >> 16) + (rgb >> 8)) & 0xFF))

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
 * ``Gradient'' filter for 24-bit color samples.
 * Should be called only when redMax, greenMax and blueMax are 255.
 * Color components assumed to be byte-aligned.
 */

static void
FilterGradient24(rfbClientPtr cl, char *buf, rfbPixelFormat *fmt, int w, int h)
{
    uint32_t *buf32;
    uint32_t pix32;
    int *prevRowPtr;
    int shiftBits[3];
    int pixHere[3], pixUpper[3], pixLeft[3], pixUpperLeft[3];
    int prediction;
    int x, y, c;

    buf32 = (uint32_t *)buf;
    memset (prevRowBuf, 0, w * 3 * sizeof(int));

    if (!cl->screen->serverFormat.bigEndian == !fmt->bigEndian) {
        shiftBits[0] = fmt->redShift;
        shiftBits[1] = fmt->greenShift;
        shiftBits[2] = fmt->blueShift;
    } else {
        shiftBits[0] = 24 - fmt->redShift;
        shiftBits[1] = 24 - fmt->greenShift;
        shiftBits[2] = 24 - fmt->blueShift;
    }

    for (y = 0; y < h; y++) {
        for (c = 0; c < 3; c++) {
            pixUpper[c] = 0;
            pixHere[c] = 0;
        }
        prevRowPtr = prevRowBuf;
        for (x = 0; x < w; x++) {
            pix32 = *buf32++;
            for (c = 0; c < 3; c++) {
                pixUpperLeft[c] = pixUpper[c];
                pixLeft[c] = pixHere[c];
                pixUpper[c] = *prevRowPtr;
                pixHere[c] = (int)(pix32 >> shiftBits[c] & 0xFF);
                *prevRowPtr++ = pixHere[c];

                prediction = pixLeft[c] + pixUpper[c] - pixUpperLeft[c];
                if (prediction < 0) {
                    prediction = 0;
                } else if (prediction > 0xFF) {
                    prediction = 0xFF;
                }
                *buf++ = (char)(pixHere[c] - prediction);
            }
        }
    }
}


/*
 * ``Gradient'' filter for other color depths.
 */

#define DEFINE_GRADIENT_FILTER_FUNCTION(bpp)                             \
                                                                         \
static void                                                              \
FilterGradient##bpp(rfbClientPtr cl, uint##bpp##_t *buf,                 \
		rfbPixelFormat *fmt, int w, int h) {                     \
    uint##bpp##_t pix, diff;                                             \
    rfbBool endianMismatch;                                              \
    int *prevRowPtr;                                                     \
    int maxColor[3], shiftBits[3];                                       \
    int pixHere[3], pixUpper[3], pixLeft[3], pixUpperLeft[3];            \
    int prediction;                                                      \
    int x, y, c;                                                         \
                                                                         \
    memset (prevRowBuf, 0, w * 3 * sizeof(int));                         \
                                                                         \
    endianMismatch = (!cl->screen->serverFormat.bigEndian != !fmt->bigEndian);    \
                                                                         \
    maxColor[0] = fmt->redMax;                                           \
    maxColor[1] = fmt->greenMax;                                         \
    maxColor[2] = fmt->blueMax;                                          \
    shiftBits[0] = fmt->redShift;                                        \
    shiftBits[1] = fmt->greenShift;                                      \
    shiftBits[2] = fmt->blueShift;                                       \
                                                                         \
    for (y = 0; y < h; y++) {                                            \
        for (c = 0; c < 3; c++) {                                        \
            pixUpper[c] = 0;                                             \
            pixHere[c] = 0;                                              \
        }                                                                \
        prevRowPtr = prevRowBuf;                                         \
        for (x = 0; x < w; x++) {                                        \
            pix = *buf;                                                  \
            if (endianMismatch) {                                        \
                pix = Swap##bpp(pix);                                    \
            }                                                            \
            diff = 0;                                                    \
            for (c = 0; c < 3; c++) {                                    \
                pixUpperLeft[c] = pixUpper[c];                           \
                pixLeft[c] = pixHere[c];                                 \
                pixUpper[c] = *prevRowPtr;                               \
                pixHere[c] = (int)(pix >> shiftBits[c] & maxColor[c]);   \
                *prevRowPtr++ = pixHere[c];                              \
                                                                         \
                prediction = pixLeft[c] + pixUpper[c] - pixUpperLeft[c]; \
                if (prediction < 0) {                                    \
                    prediction = 0;                                      \
                } else if (prediction > maxColor[c]) {                   \
                    prediction = maxColor[c];                            \
                }                                                        \
                diff |= ((pixHere[c] - prediction) & maxColor[c])        \
                    << shiftBits[c];                                     \
            }                                                            \
            if (endianMismatch) {                                        \
                diff = Swap##bpp(diff);                                  \
            }                                                            \
            *buf++ = diff;                                               \
        }                                                                \
    }                                                                    \
}

DEFINE_GRADIENT_FILTER_FUNCTION(16)
DEFINE_GRADIENT_FILTER_FUNCTION(32)


/*
 * Code to guess if given rectangle is suitable for smooth image
 * compression (by applying "gradient" filter or JPEG coder).
 */

#define JPEG_MIN_RECT_SIZE  4096

#define DETECT_SUBROW_WIDTH    7
#define DETECT_MIN_WIDTH       8
#define DETECT_MIN_HEIGHT      8

static int
DetectSmoothImage (rfbClientPtr cl, rfbPixelFormat *fmt, int w, int h)
{
    long avgError;

    if ( cl->screen->serverFormat.bitsPerPixel == 8 || fmt->bitsPerPixel == 8 ||
         w < DETECT_MIN_WIDTH || h < DETECT_MIN_HEIGHT ) {
        return 0;
    }

    if (qualityLevel != -1) {
        if (w * h < JPEG_MIN_RECT_SIZE) {
            return 0;
        }
    } else {
        if ( rfbTightDisableGradient ||
             w * h < tightConf[compressLevel].gradientMinRectSize ) {
            return 0;
        }
    }

    if (fmt->bitsPerPixel == 32) {
        if (usePixelFormat24) {
            avgError = DetectSmoothImage24(cl, fmt, w, h);
            if (qualityLevel != -1) {
                return (avgError < tightConf[qualityLevel].jpegThreshold24);
            }
            return (avgError < tightConf[compressLevel].gradientThreshold24);
        } else {
            avgError = DetectSmoothImage32(cl, fmt, w, h);
        }
    } else {
        avgError = DetectSmoothImage16(cl, fmt, w, h);
    }
    if (qualityLevel != -1) {
        return (avgError < tightConf[qualityLevel].jpegThreshold);
    }
    return (avgError < tightConf[compressLevel].gradientThreshold);
}

static unsigned long
DetectSmoothImage24 (rfbClientPtr cl,
                     rfbPixelFormat *fmt,
                     int w,
                     int h)
{
    int off;
    int x, y, d, dx, c;
    int diffStat[256];
    int pixelCount = 0;
    int pix, left[3];
    unsigned long avgError;

    /* If client is big-endian, color samples begin from the second
       byte (offset 1) of a 32-bit pixel value. */
    off = (fmt->bigEndian != 0);

    memset(diffStat, 0, 256*sizeof(int));

    y = 0, x = 0;
    while (y < h && x < w) {
        for (d = 0; d < h - y && d < w - x - DETECT_SUBROW_WIDTH; d++) {
            for (c = 0; c < 3; c++) {
                left[c] = (int)tightBeforeBuf[((y+d)*w+x+d)*4+off+c] & 0xFF;
            }
            for (dx = 1; dx <= DETECT_SUBROW_WIDTH; dx++) {
                for (c = 0; c < 3; c++) {
                    pix = (int)tightBeforeBuf[((y+d)*w+x+d+dx)*4+off+c] & 0xFF;
                    diffStat[abs(pix - left[c])]++;
                    left[c] = pix;
                }
                pixelCount++;
            }
        }
        if (w > h) {
            x += h;
            y = 0;
        } else {
            x = 0;
            y += w;
        }
    }

    if (diffStat[0] * 33 / pixelCount >= 95)
        return 0;

    avgError = 0;
    for (c = 1; c < 8; c++) {
        avgError += (unsigned long)diffStat[c] * (unsigned long)(c * c);
        if (diffStat[c] == 0 || diffStat[c] > diffStat[c-1] * 2)
            return 0;
    }
    for (; c < 256; c++) {
        avgError += (unsigned long)diffStat[c] * (unsigned long)(c * c);
    }
    avgError /= (pixelCount * 3 - diffStat[0]);

    return avgError;
}

#define DEFINE_DETECT_FUNCTION(bpp)                                          \
                                                                             \
static unsigned long                                                         \
DetectSmoothImage##bpp (rfbClientPtr cl, rfbPixelFormat *fmt, int w, int h) {\
    rfbBool endianMismatch;                                                  \
    uint##bpp##_t pix;                                                       \
    int maxColor[3], shiftBits[3];                                           \
    int x, y, d, dx, c;                                                      \
    int diffStat[256];                                                       \
    int pixelCount = 0;                                                      \
    int sample, sum, left[3];                                                \
    unsigned long avgError;                                                  \
                                                                             \
    endianMismatch = (!cl->screen->serverFormat.bigEndian != !fmt->bigEndian); \
                                                                             \
    maxColor[0] = fmt->redMax;                                               \
    maxColor[1] = fmt->greenMax;                                             \
    maxColor[2] = fmt->blueMax;                                              \
    shiftBits[0] = fmt->redShift;                                            \
    shiftBits[1] = fmt->greenShift;                                          \
    shiftBits[2] = fmt->blueShift;                                           \
                                                                             \
    memset(diffStat, 0, 256*sizeof(int));                                    \
                                                                             \
    y = 0, x = 0;                                                            \
    while (y < h && x < w) {                                                 \
        for (d = 0; d < h - y && d < w - x - DETECT_SUBROW_WIDTH; d++) {     \
            pix = ((uint##bpp##_t *)tightBeforeBuf)[(y+d)*w+x+d];            \
            if (endianMismatch) {                                            \
                pix = Swap##bpp(pix);                                        \
            }                                                                \
            for (c = 0; c < 3; c++) {                                        \
                left[c] = (int)(pix >> shiftBits[c] & maxColor[c]);          \
            }                                                                \
            for (dx = 1; dx <= DETECT_SUBROW_WIDTH; dx++) {                  \
                pix = ((uint##bpp##_t *)tightBeforeBuf)[(y+d)*w+x+d+dx];     \
                if (endianMismatch) {                                        \
                    pix = Swap##bpp(pix);                                    \
                }                                                            \
                sum = 0;                                                     \
                for (c = 0; c < 3; c++) {                                    \
                    sample = (int)(pix >> shiftBits[c] & maxColor[c]);       \
                    sum += abs(sample - left[c]);                            \
                    left[c] = sample;                                        \
                }                                                            \
                if (sum > 255)                                               \
                    sum = 255;                                               \
                diffStat[sum]++;                                             \
                pixelCount++;                                                \
            }                                                                \
        }                                                                    \
        if (w > h) {                                                         \
            x += h;                                                          \
            y = 0;                                                           \
        } else {                                                             \
            x = 0;                                                           \
            y += w;                                                          \
        }                                                                    \
    }                                                                        \
                                                                             \
    if ((diffStat[0] + diffStat[1]) * 100 / pixelCount >= 90)                \
        return 0;                                                            \
                                                                             \
    avgError = 0;                                                            \
    for (c = 1; c < 8; c++) {                                                \
        avgError += (unsigned long)diffStat[c] * (unsigned long)(c * c);     \
        if (diffStat[c] == 0 || diffStat[c] > diffStat[c-1] * 2)             \
            return 0;                                                        \
    }                                                                        \
    for (; c < 256; c++) {                                                   \
        avgError += (unsigned long)diffStat[c] * (unsigned long)(c * c);     \
    }                                                                        \
    avgError /= (pixelCount - diffStat[0]);                                  \
                                                                             \
    return avgError;                                                         \
}

DEFINE_DETECT_FUNCTION(16)
DEFINE_DETECT_FUNCTION(32)


/*
 * JPEG compression stuff.
 */

static TLS struct jpeg_destination_mgr jpegDstManager;
static TLS rfbBool jpegError = FALSE;
static TLS int jpegDstDataLen = 0;

static rfbBool
SendJpegRect(rfbClientPtr cl, int x, int y, int w, int h, int quality)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    uint8_t *srcBuf;
    JSAMPROW rowPointer[1];
    int dy;

    if (cl->screen->serverFormat.bitsPerPixel == 8)
        return SendFullColorRect(cl, w, h);

    srcBuf = (uint8_t *)malloc(w * 3);
    if (srcBuf == NULL) {
        return SendFullColorRect(cl, w, h);
    }
    rowPointer[0] = srcBuf;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    cinfo.image_width = w;
    cinfo.image_height = h;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    JpegSetDstManager (&cinfo);

    jpeg_start_compress(&cinfo, TRUE);

    for (dy = 0; dy < h; dy++) {
        PrepareRowForJpeg(cl, srcBuf, x, y + dy, w);
        jpeg_write_scanlines(&cinfo, rowPointer, 1);
        if (jpegError)
            break;
    }

    if (!jpegError)
        jpeg_finish_compress(&cinfo);

    jpeg_destroy_compress(&cinfo);
    free(srcBuf);

    if (jpegError)
        return SendFullColorRect(cl, w, h);

    if (cl->ublen + TIGHT_MIN_TO_COMPRESS + 1 > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    cl->updateBuf[cl->ublen++] = (char)(rfbTightJpeg << 4);
    rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 1);

    return SendCompressedData(cl, jpegDstDataLen);
}

static void
PrepareRowForJpeg(rfbClientPtr cl,
                  uint8_t *dst,
                  int x,
                  int y,
                  int count)
{
    if (cl->screen->serverFormat.bitsPerPixel == 32) {
        if ( cl->screen->serverFormat.redMax == 0xFF &&
             cl->screen->serverFormat.greenMax == 0xFF &&
             cl->screen->serverFormat.blueMax == 0xFF ) {
            PrepareRowForJpeg24(cl, dst, x, y, count);
        } else {
            PrepareRowForJpeg32(cl, dst, x, y, count);
        }
    } else {
        /* 16 bpp assumed. */
        PrepareRowForJpeg16(cl, dst, x, y, count);
    }
}

static void
PrepareRowForJpeg24(rfbClientPtr cl,
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
PrepareRowForJpeg##bpp(rfbClientPtr cl, uint8_t *dst, int x, int y, int count) { \
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
 * Destination manager implementation for JPEG library.
 */

static void
JpegInitDestination(j_compress_ptr cinfo)
{
    jpegError = FALSE;
    jpegDstManager.next_output_byte = (JOCTET *)tightAfterBuf;
    jpegDstManager.free_in_buffer = (size_t)tightAfterBufSize;
}

static boolean
JpegEmptyOutputBuffer(j_compress_ptr cinfo)
{
    jpegError = TRUE;
    jpegDstManager.next_output_byte = (JOCTET *)tightAfterBuf;
    jpegDstManager.free_in_buffer = (size_t)tightAfterBufSize;

    return TRUE;
}

static void
JpegTermDestination(j_compress_ptr cinfo)
{
    jpegDstDataLen = tightAfterBufSize - jpegDstManager.free_in_buffer;
}

static void
JpegSetDstManager(j_compress_ptr cinfo)
{
    jpegDstManager.init_destination = JpegInitDestination;
    jpegDstManager.empty_output_buffer = JpegEmptyOutputBuffer;
    jpegDstManager.term_destination = JpegTermDestination;
    cinfo->dest = &jpegDstManager;
}

