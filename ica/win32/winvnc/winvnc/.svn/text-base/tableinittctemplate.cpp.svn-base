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

#if !defined(OUTVNC)
#error "This file shouldn't be compiled."
#error "It is included as part of translate.c"
#endif

#define OUT_T CONCAT2E(CARD,OUTVNC)
#define SwapOUTVNC(x) CONCAT2E(Swap,OUTVNC) (x)
#define rfbInitTrueColourSingleTableOUTVNC \
				CONCAT2E(rfbInitTrueColourSingleTable,OUTVNC)
#define rfbInitTrueColourRGBTablesOUTVNC CONCAT2E(rfbInitTrueColourRGBTables,OUTVNC)
#define rfbInitOneRGBTableOUTVNC CONCAT2E(rfbInitOneRGBTable,OUTVNC)

static void
rfbInitOneRGBTableOUTVNC (OUT_T *table, int inMax, int outMax, int outShift,
		       int swap);


/*
 * rfbInitTrueColourSingleTable sets up a single lookup table for truecolour
 * translation.
 */

static void
rfbInitTrueColourSingleTableOUTVNC (char **table, rfbPixelFormat *in,
				 rfbPixelFormat *out)
{
    int i;
    int inRed, inGreen, inBlue, outRed, outGreen, outBlue;
    OUT_T *t;
    int nEntries = 1 << in->bitsPerPixel;

    if (*table) free(*table);
    *table = (char *)malloc(nEntries * sizeof(OUT_T));
	if (table == NULL) return;
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
#if (OUTVNC != 8)
	if (out->bigEndian != in->bigEndian) {
	    t[i] = SwapOUTVNC(t[i]);
	}
#endif
    }
}


/*
 * rfbInitTrueColourRGBTables sets up three separate lookup tables for the
 * red, green and blue values.
 */

static void
rfbInitTrueColourRGBTablesOUTVNC (char **table, rfbPixelFormat *in,
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

    rfbInitOneRGBTableOUTVNC (redTable, in->redMax, out->redMax,
			   out->redShift, (out->bigEndian != in->bigEndian));
    rfbInitOneRGBTableOUTVNC (greenTable, in->greenMax, out->greenMax,
			   out->greenShift, (out->bigEndian != in->bigEndian));
    rfbInitOneRGBTableOUTVNC (blueTable, in->blueMax, out->blueMax,
			   out->blueShift, (out->bigEndian != in->bigEndian));
}

static void
rfbInitOneRGBTableOUTVNC (OUT_T *table, int inMax, int outMax, int outShift,
		       int swap)
{
    int i;
    int nEntries = inMax + 1;

    for (i = 0; i < nEntries; i++) {
	table[i] = ((i * outMax + inMax / 2) / inMax) << outShift;
#if (OUTVNC != 8)
	if (swap) {
	    table[i] = SwapOUTVNC(table[i]);
	}
#endif
    }
}

#undef OUT_T
#undef SwapOUTVNC
#undef rfbInitTrueColourSingleTableOUTVNC
#undef rfbInitTrueColourRGBTablesOUTVNC
#undef rfbInitOneRGBTableOUTVNC
