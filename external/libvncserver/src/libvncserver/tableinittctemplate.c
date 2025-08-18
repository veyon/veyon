/*
 * tableinittctemplate.c - template for initialising lookup tables for
 * truecolour to truecolour translation.
 *
 * This file shouldn't be compiled.  It is included multiple times by
 * translate.c, each time with a different definition of the macro OUT.
 * For each value of OUT, this file defines two functions for initialising
 * lookup tables.  One is for truecolour translation using a single lookup
 * table, the other is for truecolour translation using three separate
 * lookup tables for the red, green and blue values.
 *
 * I know this code isn't nice to read because of all the macros, but
 * efficiency is important here.
 */

/*
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
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

#if !defined(OUT)
#error "This file shouldn't be compiled."
#error "It is included as part of translate.c"
#endif

#define OUT_T CONCAT3E(uint,OUT,_t)
#define SwapOUT(x) CONCAT2E(Swap,OUT(x))
#define rfbInitTrueColourSingleTableOUT \
                                CONCAT2E(rfbInitTrueColourSingleTable,OUT)
#define rfbInitTrueColourRGBTablesOUT CONCAT2E(rfbInitTrueColourRGBTables,OUT)
#define rfbInitOneRGBTableOUT CONCAT2E(rfbInitOneRGBTable,OUT)

static void
rfbInitOneRGBTableOUT (OUT_T *table, int inMax, int outMax, int outShift,
                       int swap);


/*
 * rfbInitTrueColourSingleTable sets up a single lookup table for truecolour
 * translation.
 */

static void
rfbInitTrueColourSingleTableOUT (char **table, rfbPixelFormat *in,
                                 rfbPixelFormat *out)
{
    int i;
    int inRed, inGreen, inBlue, outRed, outGreen, outBlue;
    OUT_T *t;
    int nEntries = 1 << in->bitsPerPixel;

    if (*table) free(*table);
    *table = (char *)malloc(nEntries * sizeof(OUT_T));
    t = (OUT_T *)*table;

    for (i = 0; i < nEntries; i++) {
        inRed   = (i >> in->redShift)   & in->redMax;
        inGreen = (i >> in->greenShift) & in->greenMax;
        inBlue  = (i >> in->blueShift)  & in->blueMax;

        outRed   = (inRed   * out->redMax   + in->redMax / 2)   / in->redMax;
        outGreen = (inGreen * out->greenMax + in->greenMax / 2) / in->greenMax;
        outBlue  = (inBlue  * out->blueMax  + in->blueMax / 2)  / in->blueMax;

        t[i] = ((outRed   << out->redShift)   |
                (outGreen << out->greenShift) |
                (outBlue  << out->blueShift));
#if (OUT != 8)
        if (out->bigEndian != in->bigEndian) {
            t[i] = SwapOUT(t[i]);
        }
#endif
    }
}


/*
 * rfbInitTrueColourRGBTables sets up three separate lookup tables for the
 * red, green and blue values.
 */

static void
rfbInitTrueColourRGBTablesOUT (char **table, rfbPixelFormat *in,
                               rfbPixelFormat *out)
{
    OUT_T *redTable;
    OUT_T *greenTable;
    OUT_T *blueTable;

    if (*table) free(*table);
    *table = (char *)malloc((in->redMax + in->greenMax + in->blueMax + 3)
                            * sizeof(OUT_T));
    redTable = (OUT_T *)*table;
    greenTable = redTable + in->redMax + 1;
    blueTable = greenTable + in->greenMax + 1;

    rfbInitOneRGBTableOUT (redTable, in->redMax, out->redMax,
                           out->redShift, (out->bigEndian != in->bigEndian));
    rfbInitOneRGBTableOUT (greenTable, in->greenMax, out->greenMax,
                           out->greenShift, (out->bigEndian != in->bigEndian));
    rfbInitOneRGBTableOUT (blueTable, in->blueMax, out->blueMax,
                           out->blueShift, (out->bigEndian != in->bigEndian));
}

static void
rfbInitOneRGBTableOUT (OUT_T *table, int inMax, int outMax, int outShift,
                       int swap)
{
    int i;
    int nEntries = inMax + 1;

    for (i = 0; i < nEntries; i++) {
        if (outShift < 32) {
            table[i] = ((OUT_T)((i * outMax + inMax / 2) / inMax)) << outShift;
        } else {
            table[i] = 0;
        }
#if (OUT != 8)
        if (swap) {
            table[i] = SwapOUT(table[i]);
        }
#endif
    }
}

#undef OUT_T
#undef SwapOUT
#undef rfbInitTrueColourSingleTableOUT
#undef rfbInitTrueColourRGBTablesOUT
#undef rfbInitOneRGBTableOUT
